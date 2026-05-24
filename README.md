
# L1_Projet
_Systeme_de_prevention_et_d'alerte_de_gel

[presentation_alerte_gel.pptx](https://github.com/user-attachments/files/28197117/presentation_alerte_gel.pptx)

# Code numéro 1 (LED + BUZZER)


/*******************************************************************************
 * Système de Prévention et d'Alerte de Gel pour Agriculteurs
 * Carte : RFThings UCA Education Board v3.8
 * Capteur : SHTC3 (température + humidité)
 * Communication : LoRaWAN ABP via TTN
 * Alertes : LEDs RGB + Buzzer
 *******************************************************************************/

#define CFG_EU 1

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <Wire.h>
#include "SHTSensor.h"      // http://librarymanager/All#arduino_shtc3
#include <FastLED.h>        // http://librarymanager/All#FASTLED

// ---------- LEDs RGB ----------
#define LED_PIN         4
#define NUM_LEDS        21
#define BRIGHTNESS      64
#define LED_TYPE        WS2811
#define COLOR_ORDER     GRB
CRGB leds[NUM_LEDS];

// ---------- Buzzer ----------
#define BUZZER_PIN      5

// ---------- Seuils d'alerte ----------
#define SEUIL_ALERTE    3.0     // °C → Alerte niveau 1 (LEDs orange)
#define SEUIL_GEL       0.0     // °C → Alerte niveau 2 (LEDs rouges + Buzzer)

// ---------- Capteur ----------
SHTSensor sht;

// ---------- Clés TTN (ABP) ----------
static const u4_t DEVADDR = 0x260BB493;

static const PROGMEM u1_t NWKSKEY[16] = { 0x0A, 0x35, 0xF9, 0xF5, 0x3E, 0xF9, 0x60, 0xEC, 0x43, 0x32, 0x98, 0xEB, 0x11, 0xCD, 0x99, 0x9D };

static const u1_t PROGMEM APPSKEY[16] = { 0x07, 0xD5, 0x31, 0x19, 0xA8, 0xF3, 0xD8, 0x0F, 0xEE, 0xD7, 0x36, 0x06, 0x4A, 0x5A, 0xB6, 0x86 };

// ---------- LoRaWAN ----------
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

static osjob_t sendjob;
const unsigned TX_INTERVAL = 60;

const lmic_pinmap lmic_pins = {
    .nss = 10,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 8,
    .dio = {6, 6, 6},
};

// ---------- Variables globales ----------
float temperature = 0;
float humidite = 0;

// ============================================================
// Fonction : Allumer toutes les LEDs d'une couleur
// ============================================================
void allumerLEDs(CRGB couleur) {
    fill_solid(leds, NUM_LEDS, couleur);
    FastLED.show();
}

// ============================================================
// Fonction : Activer le buzzer (volume maximum)
// ============================================================
void activerBuzzer() {
    for (int i = 0; i < 5; i++) {
        // Bips rapides pour maximiser le volume
        for (int j = 0; j < 100; j++) {
            digitalWrite(BUZZER_PIN, HIGH);
            delayMicroseconds(500);
            digitalWrite(BUZZER_PIN, LOW);
            delayMicroseconds(500);
        }
        delay(200);
    }
}

// ============================================================
// Fonction : Gérer les alertes selon la température
// ============================================================
void gererAlertes() {
    if (temperature <= SEUIL_GEL) {
        // 🚨 ALERTE NIVEAU 2 : GEL CRITIQUE
        Serial.println("ALERTE NIVEAU 2 : GEL CRITIQUE !");
        allumerLEDs(CRGB::Red);
        activerBuzzer();
    } else if (temperature <= SEUIL_ALERTE) {
        // ⚠️ ALERTE NIVEAU 1 : RISQUE DE GEL
        Serial.println("ALERTE NIVEAU 1 : Risque de gel !");
        allumerLEDs(CRGB::Orange);
        digitalWrite(BUZZER_PIN, LOW);
    } else {
        // ✅ NORMAL
        Serial.println("Temperature normale - Pas de risque de gel");
        allumerLEDs(CRGB::Green);
        digitalWrite(BUZZER_PIN, LOW);
    }
}

// ============================================================
// Callback LoRaWAN
// ============================================================
void onEvent (ev_t ev) {
    switch(ev) {
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE"));
            if (LMIC.txrxFlags & TXRX_ACK)
                Serial.println(F("Received ack"));
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
        default:
            break;
    }
}

// ============================================================
// Fonction : Envoyer les données via LoRaWAN
// ============================================================
void do_send(osjob_t* j) {
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        // Lire le capteur
        sht.readSample();
        temperature = sht.getTemperature();
        humidite    = sht.getHumidity();

        // Affichage série
        Serial.println("--------------------------------------------");
        Serial.print("[SHTC3] Temperature : ");
        Serial.print(temperature);
        Serial.println(" C");
        Serial.print("[SHTC3] Humidite    : ");
        Serial.print(humidite);
        Serial.println(" %");

        // Gérer les alertes
        gererAlertes();

        // Préparer le payload (température * 100 et humidité * 100 en entiers)
        int16_t temp_int = (int16_t)(temperature * 100);
        int16_t hum_int  = (int16_t)(humidite * 100);

        uint8_t mydata[4];
        mydata[0] = temp_int >> 8;
        mydata[1] = temp_int & 0xFF;
        mydata[2] = hum_int >> 8;
        mydata[3] = hum_int & 0xFF;

        // Envoyer via LoRaWAN
        LMIC_setTxData2(1, mydata, sizeof(mydata), 0);
        Serial.println(F("[LoRaWAN] Paquet envoye !"));
    }
}

// ============================================================
void setup() {
    Serial.begin(115200);

    // --- LEDs ---
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(BRIGHTNESS);
    allumerLEDs(CRGB::Blue);

    // --- Buzzer ---
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);

    // --- Capteur SHTC3 ---
    Wire.begin();
    if (!sht.init()) {
        Serial.println("Erreur : capteur SHTC3 non detecte !");
        allumerLEDs(CRGB::Red);
        while (1);
    }
    sht.setAccuracy(SHTSensor::SHT_ACCURACY_MEDIUM);
    Serial.println("Capteur SHTC3 initialise OK");

    // --- LoRaWAN LMIC ---
    os_init();
    LMIC_reset();
    LMIC_setClockError(MAX_CLOCK_ERROR * 2 / 100);

    uint8_t appskey[sizeof(APPSKEY)];
    uint8_t nwkskey[sizeof(NWKSKEY)];
    memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
    memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
    LMIC_setSession(0x1, DEVADDR, nwkskey, appskey);

    // Canaux EU868
    LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);
    LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);
    LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);

    LMIC_setLinkCheckMode(0);
    LMIC.dn2Dr = DR_SF9;
    LMIC_setDrTxpow(DR_SF9, 20);

    Serial.println("============================================");
    Serial.println("  Systeme de prevention de gel demarre  ");
    Serial.println("============================================");

    // Premier envoi
    do_send(&sendjob);
}

// ============================================================
void loop() {
    os_runloop_once();
}
