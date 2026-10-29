/**
 * =============================================================================
 * PROGRAM PENGUJIAN: Saklar Toggle Sensor (Master Enable Otomasi)
 * + FITUR WIFI MANAGER & ARDUINO OTA (OVER-THE-AIR)
 * Proyek: Smart City Miniatur Diorama (ESP32 DevKit v1)
 * =============================================================================
 *
 * Alokasi Pin:
 * - Saklar Toggle Sensor : GPIO 15 (INPUT_PULLUP) -> Soket J_SW_SENS (JST 2-Pin)
 *                          * Pin 1 : GPIO 15
 *                          * Pin 2 : GND
 *
 * Pin Indikator Visual (Opsional):
 * - Lampu Rumah 1 & 2    : GPIO 18 (OUTPUT) -> Ikut menyala saat saklar ON
 *
 * Karakteristik INPUT_PULLUP:
 * - Saklar POSISI ON  (Terhubung ke GND) : digitalRead(15) = LOW (0)  -> Otomasi Sensor AKTIF
 * - Saklar POSISI OFF (Terbuka / Floating): digitalRead(15) = HIGH (1) -> Otomasi Sensor NON-AKTIF
 * =============================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// Definisi Pin
constexpr uint8_t PIN_SW_SENS   = 15; // Saklar Toggle SPST (Input Pull-up)
constexpr uint8_t PIN_INDICATOR = 18; // Indikator visual ke Lampu Rumah 1-2

// Variabel status
int lastSwitchState = -1;
unsigned long lastPrintTime = 0;
constexpr unsigned long PRINT_INTERVAL_MS = 500;

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Konfigurasi Pin
    pinMode(PIN_SW_SENS, INPUT_PULLUP);
    pinMode(PIN_INDICATOR, OUTPUT);

    digitalWrite(PIN_INDICATOR, LOW);

    Serial.println("\n========================================================");
    Serial.println("   PENGUJIAN SAKLAR TOGGLE SENSOR (MASTER ENABLE)       ");
    Serial.println("        DILENGKAPI WIFIMANAGER & ARDUINO OTA            ");
    Serial.println("========================================================");

    // -------------------------------------------------------------------------
    // INISIALISASI WIFIMANAGER & ARDUINO OTA
    // -------------------------------------------------------------------------
    WiFiManager wm;
    wm.setConfigPortalTimeout(180);

    Serial.println("[WIFI] Memulai koneksi via WiFiManager...");
    bool isConnected = wm.autoConnect("SmartCity-Switch-AP");

    if (!isConnected) {
        Serial.println("[WIFI] Timeout portal tercapai / Gagal konek. Mode Offline aktif!");
    } else {
        Serial.println("[WIFI] Berhasil terhubung ke Wi-Fi!");
        Serial.print("[WIFI] Alamat IP: ");
        Serial.println(WiFi.localIP());

        ArduinoOTA.setHostname("smartcity-toggle");

        ArduinoOTA.onStart([]() {
            String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
            Serial.println("\n[OTA] Memulai transfer firmware Over-The-Air: " + type);
            digitalWrite(PIN_INDICATOR, LOW);
        });

        ArduinoOTA.onEnd([]() {
            Serial.println("\n[OTA] Update Berhasil! ESP32 me-restart...");
        });

        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            Serial.printf("[OTA] Progress Transfer: %u%%\r", (progress / (total / 100)));
        });

        ArduinoOTA.onError([](ota_error_t error) {
            Serial.printf("[OTA] Error [%u]\n", error);
        });

        ArduinoOTA.begin();
        Serial.println("[OTA] Service ArduinoOTA Siap!");
        Serial.printf(">>> PERINTAH UPLOAD VIA WI-FI (OTA):\n");
        Serial.printf("    pio run -e test_toggle -t upload --upload-port %s\n", WiFi.localIP().toString().c_str());
        Serial.println("--------------------------------------------------------");
    }

    Serial.println("Mulai pengujian saklar toggle...\n");
}

void loop() {
    // 0. Handle OTA jika Wi-Fi terhubung
    if (WiFi.status() == WL_CONNECTED) {
        ArduinoOTA.handle();
    }

    // Reset Wi-Fi via Serial Monitor (Ketik 'r' lalu Enter)
    if (Serial.available()) {
        char c = Serial.read();
        if (c == 'r' || c == 'R') {
            Serial.println("\n[WIFI] Mereset kredensial Wi-Fi...");
            WiFiManager wm;
            wm.resetSettings();
            delay(1000);
            ESP.restart();
        }
    }

    unsigned long now = millis();

    // Baca status saklar (LOW = ON / Tertutup ke GND)
    int switchState = digitalRead(PIN_SW_SENS);
    bool isEnabled = (switchState == LOW);

    // Kontrol indikator visual
    digitalWrite(PIN_INDICATOR, isEnabled ? HIGH : LOW);

    // Event Trigger jika saklar digeser
    if (switchState != lastSwitchState) {
        lastSwitchState = switchState;
        Serial.println("\n---------------- [PERUBAHAN POSISI SAKLAR] ----------------");
        if (isEnabled) {
            Serial.println(">>> SAKLAR: [ ON ] (GPIO 15: LOW)  -> OTOMASI SENSOR: AKTIF (ENABLE)   <<<");
            Serial.println(">>> LED Indikator (GPIO 18) MENYALA.                                     <<<");
        } else {
            Serial.println(">>> SAKLAR: [ OFF ] (GPIO 15: HIGH) -> OTOMASI SENSOR: NON-AKTIF (DISABLE) <<<");
            Serial.println(">>> LED Indikator (GPIO 18) PADAM.                                       <<<");
        }
        Serial.println("-----------------------------------------------------------\n");
    }

    // Monitoring Serial periodik
    if (now - lastPrintTime >= PRINT_INTERVAL_MS) {
        lastPrintTime = now;
        Serial.printf("[SWITCH D15] Nilai:%d (%-4s) | Status:%-10s | Indikator:%-3s | IP:%s\n",
                      switchState,
                      (switchState == LOW) ? "LOW" : "HIGH",
                      isEnabled ? "AKTIF" : "NON-AKTIF",
                      isEnabled ? "ON " : "OFF",
                      (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString().c_str() : "OFFLINE");
    }

    delay(20); // Debounce sampling saklar mekanik
}
