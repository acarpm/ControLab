#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_PN532.h>

// ==========================================
// 1. CONFIGURATION RÉSEAU & API
// ==========================================
const char *ssid = "HUAWEI-45D7";
const char *password = "72935939";

// L'URL de ton script PHP sur IONOS (utilise https si possible)
const char *serverName = "https://dev-reservation.campusfab.net/ControlLab/config/pointage.php";
String api_key = "votre_cle_api_secrete"; // Pour sécuriser ton API

// ==========================================
// 2. CONFIGURATION LECTEUR NFC (PN532 en I2C)
// ==========================================
#define SDA_PIN 21
#define SCL_PIN 22

// On passe des broches par défaut (2 et 3) pour IRQ et RESET (même si on ne les câble pas).
// Cela force la librairie à utiliser le mode "I2C Matériel" au lieu d'essayer de bidouiller les broches.
Adafruit_PN532 nfc(2, 3);

// ==========================================
// FONCTIONS UTILITAIRES
// ==========================================

// Fonction pour envoyer la donnée au serveur
void envoyerPointage(String uid_str)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;

    // Début de la connexion
    http.begin(serverName);

    // On précise qu'on envoie les données comme un formulaire web
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    // Préparation des données (ex: api_key=cle_secrete_123&uid=A1B2C3D4)
    String httpRequestData = "api_key=" + api_key + "&uid=" + uid_str;

    Serial.print("Envoi en cours pour l'UID: ");
    Serial.println(uid_str);

    // Envoi de la requête POST
    int httpResponseCode = http.POST(httpRequestData);

    if (httpResponseCode > 0)
    {
      Serial.print("Succès ! Code HTTP : ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      Serial.println("Réponse du serveur : " + payload);
    }
    else
    {
      Serial.print("Erreur lors de l'envoi HTTP. Code : ");
      Serial.println(httpResponseCode);
    }

    http.end(); // Libère les ressources
  }
  else
  {
    Serial.println("Erreur : ESP32 déconnecté du Wi-Fi");
  }
}

// ==========================================
// INITIALISATION
// ==========================================
void setup()
{
  Serial.begin(115200);
  delay(2000); // Délai plus long pour ESP32-C6

  Serial.println("\n\n=== ESP32-C6 DÉMARRÉ ===");
  Serial.println("Si vous voyez ce message, le Serial fonctionne !");

  // --- Connexion Wi-Fi ---
  Serial.print("Connexion au Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnecté au réseau ! IP: " + WiFi.localIP().toString());

  // --- Initialisation NFC ---
  Serial.println("Initialisation du bus I2C...");

  // ÉTAPE CRUCIALE POUR L'ESP32-C6 : On route le signal I2C sur les broches 21 et 22
  Wire.begin(SDA_PIN, SCL_PIN);
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata)
  {
    Serial.print("Lecteur PN532 introuvable. Vérifiez le câblage !");
    while (1)
      ; // On bloque ici si pas de lecteur
  }

  Serial.println("Lecteur NFC trouvé et prêt !");
  // Configure la carte pour lire les tags RFID classiques (Mifare)
  nfc.SAMConfig();
}

// ==========================================
// BOUCLE PRINCIPALE
// ==========================================
void loop()
{
  uint8_t success;
  uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0}; // Buffer pour stocker l'UID lu
  uint8_t uidLength;                     // Longueur de l'UID (4 ou 7 octets)

  // Attend passivement qu'une carte soit approchée
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (success)
  {
    Serial.println("\n--- CARTE DÉTECTÉE ---");

    // Conversion du tableau de bytes (uid) en String Hexadécimal lisible
    String uidString = "";
    for (uint8_t i = 0; i < uidLength; i++)
    {
      if (uid[i] < 0x10)
        uidString += "0"; // Ajoute un zéro pour garder 2 caractères (ex: 0A)
      uidString += String(uid[i], HEX);
    }

    uidString.toUpperCase(); // Met tout en majuscules (ex: a1b2 -> A1B2)

    // Envoi au serveur
    envoyerPointage(uidString);

    // Délai pour éviter de scanner la même carte 50 fois par seconde si on la laisse posée
    delay(3000);
  }
}