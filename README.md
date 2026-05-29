
# L1_Projet_Systeme_de_prevention_et_d'alerte_de_gel


Projet réalisé dans le cadre du cours **Communication Sans Fil** à l'Université Côte d'Azur de Valrose.

## 📋 Description

Système IoT de surveillance de température permettant de détecter et alerter les agriculteurs en cas de risque de gel, grâce à la technologie **LoRaWAN** et le réseau **The Things Network (TTN)**.

## 🔧 Matériel utilisé

| Composant     | Description                      |
|---------------|----------------------------------|
| Carte UCA     | RFThings Education Board v3.8    |
| Capteur SHTC3 | Température & Humidité (intégré) |
| Module LoRa   | RFM95W 868 MHz                   |
| Buzzer        | Alerte sonore (pin D5)           |

## 🚨 Système d'alertes

| Température | État            | Action                      |
|-------------|-----------------|-----------------------------|
| > 3°C       | ✅ Normal       | LEDs vertes, envoi LoRaWAN |
| ≤ 3°C       | ⚠️ Risque gel   | LEDs orange, alerte TTN    |
| ≤ 0°C       | 🚨 Gel critique | LEDs rouges + Buzzer       |

## 📡 Architecture
Carte UCA → LoRaWAN 868MHz → Gateway UniCA → TTN → Dashboard

## 📚 Bibliothèques

- `arduino-lmic` — Communication LoRaWAN
- `arduino-sht` — Capteur SHTC3
- `FastLED` — LEDs RGB

## 💻 Installation

1. Installer **Arduino IDE 1.8.19**
2. Ajouter le package RFThings dans le gestionnaire de cartes
3. Installer les bibliothèques nécessaires
4. Configurer les clés TTN (DEVADDR, NWKSKEY, APPSKEY)
5. Téléverser le code sur la carte UCA

## 🔌 Branchements

La carte UCA Education Board intègre déjà le capteur SHTC3 et les LEDs RGB, donc le seul composant externe à brancher est le **buzzer**.

### Buzzer

| Buzzer | Carte UCA |
|--------|-----------|
| **+** (côté croix) | Pin **D5** |
| **-** (autre patte) | **GND** |
