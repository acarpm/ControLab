## Fonctionnement de la carte ControlLab

### Vue d'ensemble

ControlLab est une carte de contrôle d'accès NFC.
Son rôle est de lire un badge NFC présenté par un utilisateur,
de vérifier son autorisation, puis de signaler le résultat
(LED + buzzer). 


```
Badge NFC
    │
    ▼
┌─────────┐    I2C     ┌──────────────────┐
│  PN532  │ ────────▶  │  ESP32-S3-MINI-1 │
│  (NFC)  │            │                  │
└─────────┘            │  • Lecture UID   │
                       │  • Vérification  │
┌─────────┐            │  • Wi-Fi / BLE   │
│  OLED   │ ◀───I2C─── │                  │
└─────────┘            └──────────────────┘
                              │        │
                         GPIO38    GPIO46
                              │        │
                        NeoPixels   Buzzer
```
                
### 1. Alimentation

La carte est alimentée via USB-C (VBUS = 5V).

- **Polyfuse F1** : protection contre les surintensités
- **TVS D6** : protection contre les surtensions ESD sur VBUS
- **AMS1117-3.3** : régulateur linéaire qui abaisse le 5V en 3.3V
  pour alimenter l'ESP32, le PN532, l'OLED et le buzzer.

USB-C (5V VBUS)
    │
   [F1] Polyfuse
    │
   [D6] TVS (protection ESD)
    │
[AMS1117-3.3] ──▶ 3V3 (ESP32, PN532, OLED, Buzzer)

### 2. Lecture NFC (PN532)

Le module PN532 est le lecteur NFC de la carte.
Il communique avec l'ESP32 via **I2C** (SDA = GPIO10, SCL = GPIO11).

Quand un badge ( tag NFC) est présenté :
1. Le PN532 détecte le champ RF et lit l'**UID** du badge
2. Il transmet l'UID à l'ESP32 via I2C

Les lignes RSTPD (GPIO11) et IRQ (GPIO10) permettent
à l'ESP32 de réinitialiser le PN532 et d'être notifié
d'une nouvelle lecture sans polling continu.

### 3. Signalisation utilisateur

#### LEDs NeoPixel (GPIO38)
3 LEDs RGB WS2812 montées en chaîne DIN → DOUT.
Elles indiquent l'état de la lecture :

| Couleur       | Signification         |
|---------------|-----------------------|
| 🟢 Vert       | Accès autorisé        |
| 🔴 Rouge      | Accès refusé          |
| 🔵 Bleu       | En attente / veille   |

#### Buzzer (GPIO46)
Buzzer actif 3.3V piloté via GPIO46 (R7 = 200Ω en série).
- 1 bip court : badge reconnu
- 2 bips : accès refusé

#### Écran OLED (J2 — I2C)
Affiche le nom de l'utilisateur ou le statut de la carte.
Lignes I2C partagées avec le PN532 .
Pull-ups 4.7kΩ maintenus à 3.3V (R5, R6).


### 5. Programmation

La carte se programme via **USB natif** (GPIO18/19 — USB D-/D+).
Aucun adaptateur UART externe nécessaire.
Pour passer en mode flash :
1. Maintenir **BOOT** (SW2 — GPIO0)
2. Appuyer sur **RESET** (SW1)
3. Relâcher BOOT


