#define CFG_EU 1

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <Wire.h>
#include "SHTSensor.h"

// ---------- Buzzer ----------
#define BUZZER_PIN  5

// ---------- Seuils ----------
#define SEUIL_ALERTE  3.0
#define SEUIL_GEL     0.0

// ---------- Capteur ----------
SHTSensor sht;

// ---------- Cles TTN ----------
static const u4_t DEVADDR = 0x260BB493;
static const PROGMEM u1_t NWKSKEY[16] = { 0x0A, 0x35, 0xF9, 0xF5, 0x3E, 0xF9, 0x60, 0xEC, 0x43, 0x32, 0x98, 0xEB, 0x11, 0xCD, 0x99, 0x9D };
static const u1_t PROGMEM APPSKEY[16] = { 0x07, 0xD5, 0x31, 0x19, 0xA8, 0xF3, 0xD8, 0x0F, 0xEE, 0xD7, 0x36, 0x06, 0x4A, 0x5A, 0xB6, 0x86 };

void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

static osjob_t sendjob;
const unsigned TX_INTERVAL = 15;

const lmic_pinmap lmic_pins = {
    .nss = 10,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 8,
    .dio = {6, 6, 6},
};

float temperature = 0;
float humidite = 0;

void activerBuzzer() {
    for (int i = 0; i < 3; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(100);
        digitalWrite(BUZZER_PIN, LOW);
        delay(100);
    }
}

void onEvent(ev_t ev) {
    switch(ev) {
        case EV_TXCOMPLETE:
            Serial.println(F("TX OK"));
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
        default:
            break;
    }
}

void do_send(osjob_t* j) {
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("Occupe"));
        return;
    }

    sht.readSample();
    temperature = sht.getTemperature();
    humidite    = sht.getHumidity();

    Serial.print(F("T:"));
    Serial.print(temperature);
    Serial.print(F("C H:"));
    Serial.print(humidite);
    Serial.println(F("%"));

    if (temperature <= SEUIL_GEL) {
        Serial.println(F("GEL CRITIQUE!"));
        activerBuzzer();
    } else if (temperature <= SEUIL_ALERTE) {
        Serial.println(F("RISQUE GEL!"));
        digitalWrite(BUZZER_PIN, LOW);
    } else {
        Serial.println(F("Normal"));
        digitalWrite(BUZZER_PIN, LOW);
    }

    int16_t temp_int = (int16_t)(temperature * 100);
    int16_t hum_int  = (int16_t)(humidite * 100);
    uint8_t mydata[4];
    mydata[0] = temp_int >> 8;
    mydata[1] = temp_int & 0xFF;
    mydata[2] = hum_int >> 8;
    mydata[3] = hum_int & 0xFF;

    LMIC_setTxData2(1, mydata, sizeof(mydata), 0);
    Serial.println(F("Envoi LoRaWAN..."));
}

void setup() {
    Serial.begin(115200);
    Serial.println(F("Systeme gel demarre!"));

    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);

    Wire.begin();
    sht.init();
    sht.setAccuracy(SHTSensor::SHT_ACCURACY_MEDIUM);

    os_init();
    LMIC_reset();
    LMIC_setClockError(MAX_CLOCK_ERROR * 2 / 100);

    uint8_t appskey[sizeof(APPSKEY)];
    uint8_t nwkskey[sizeof(NWKSKEY)];
    memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
    memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
    LMIC_setSession(0x1, DEVADDR, nwkskey, appskey);

    #if defined(CFG_eu868)
    LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);
    LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);
    LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);
    #endif

    LMIC_setLinkCheckMode(0);
    LMIC.dn2Dr = DR_SF9;
    LMIC_setDrTxpow(DR_SF9, 20);

    do_send(&sendjob);
}

void loop() {
    os_runloop_once();
}
