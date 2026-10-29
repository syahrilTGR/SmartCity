/**
 * =============================================================================
 * Program Pengujian Sensor IR Proximity 1 & 2 dan Lampu LPJ Jalur Atas 1 & 2
 * + FITUR WIFI MANAGER & ARDUINO OTA (OVER-THE-AIR)
 * Proyek: Smart City Miniatur Diorama (ESP32 DevKit v1)
 * =============================================================================
 *
 * Alokasi Pin:
 * - IR Sensor 1       : GPIO 34 (Input-Only)
 * - IR Sensor 2       : GPIO 35 (Input-Only)
 * - LPJ Jalur Atas 1  : GPIO 14 (Output, LED via Resistor 100 Ohm)
 * - LPJ Jalur Atas 2  : GPIO 27 (Output, LED via Resistor 100 Ohm)
 *
 * Logika Pengujian:
 * - Modul IR obstacle umumnya Active LOW:
 *   * Terdeteksi objek / mobil -> OUT = LOW (0)  -> Lampu ON (HIGH)
 *   * Tidak ada objek          -> OUT = HIGH (1) -> Lampu OFF (LOW)
 * - IR 1 (GPIO 34) mengendalikan LPJ Atas 1 (GPIO 14)
 * - IR 2 (GPIO 35) mengendalikan LPJ Atas 2 (GPIO 27)
 * =============================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// Definisi Pin Input Sensor IR
constexpr uint8_t PIN_IR1 = 34;
constexpr uint8_t PIN_IR2 = 35;

// Definisi Pin Output Lampu LPJ Atas
constexpr uint8_t PIN_LPJ_ATAS_1 = 14;
constexpr uint8_t PIN_LPJ_ATAS_2 = 27;

// Status terakhir untuk mendeteksi perubahan
int lastStateIR1 = -1;
int lastStateIR2 = -1;

unsigned long lastPrintTime = 0;
constexpr unsigned long PRINT_INTERVAL_MS = 500;

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("==================================================");
    Serial.println("   TESTING SENSOR IR 1 & IR 2 + LPJ ATAS 1 & 2   ");
    Serial.println("        DILENGKAPI WIFIMANAGER & ARDUINO OTA      ");
    Serial.println("==================================================");

    // Konfigurasi pin input (GPIO 34 & 35 adalah input-only pada ESP32)
    pinMode(PIN_IR1, INPUT);
    pinMode(PIN_IR2, INPUT);

    // Konfigurasi pin output LED
    pinMode(PIN_LPJ_ATAS_1, OUTPUT);
    pinMode(PIN_LPJ_ATAS_2, OUTPUT);

    // Inisialisasi awal lampu MATI
    digitalWrite(PIN_LPJ_ATAS_1, LOW);
    digitalWrite(PIN_LPJ_ATAS_2, LOW);

    // -------------------------------------------------------------------------
    // INISIALISASI WIFIMANAGER & ARDUINO OTA
    // -------------------------------------------------------------------------
    WiFiManager wm;
    wm.setConfigPortalTimeout(180);

    Serial.println("[WIFI] Memulai koneksi via WiFiManager...");
    bool isConnected = wm.autoConnect("SmartCity-IR-AP");

    if (!isConnected) {
        Serial.println("[WIFI] Timeout portal tercapai / Gagal konek. Mode Offline aktif!");
    } else {
        Serial.println("[WIFI] Berhasil terhubung ke Wi-Fi!");
        Serial.print("[WIFI] Alamat IP: ");
        Serial.println(WiFi.localIP());

        ArduinoOTA.setHostname("smartcity-ir");

        ArduinoOTA.onStart([]() {
            String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
            Serial.println("\n[OTA] Memulai transfer firmware Over-The-Air: " + type);
            digitalWrite(PIN_LPJ_ATAS_1, LOW);
            digitalWrite(PIN_LPJ_ATAS_2, LOW);
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
        Serial.printf("    pio run -e test_ir -t upload --upload-port %s\n", WiFi.localIP().toString().c_str());
        Serial.println("--------------------------------------------------");
    }

    Serial.println("Mulai pengujian... Letakkan tangan/objek di depan IR!\n");
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

    // Baca status digital sensor (modul IR umumnya LOW saat terdeteksi halangan)
    int stateIR1 = digitalRead(PIN_IR1);
    int stateIR2 = digitalRead(PIN_IR2);

    // Logika IR 1 -> LPJ Atas 1
    bool ir1Detected = (stateIR1 == LOW);
    digitalWrite(PIN_LPJ_ATAS_1, ir1Detected ? HIGH : LOW);

    // Logika IR 2 -> LPJ Atas 2
    bool ir2Detected = (stateIR2 == LOW);
    digitalWrite(PIN_LPJ_ATAS_2, ir2Detected ? HIGH : LOW);

    // Cetak log jika terjadi perubahan status
    if (stateIR1 != lastStateIR1 || stateIR2 != lastStateIR2) {
        Serial.print("[TRIGGER] ");
        Serial.print("IR 1: ");
        Serial.print(ir1Detected ? "DETEKSI OBJEK (LOW)" : "BEBAS (HIGH)");
        Serial.print(" -> LPJ 1: ");
        Serial.print(ir1Detected ? "MENYALA (ON)" : "MATI (OFF)");
        Serial.print(" | ");
        Serial.print("IR 2: ");
        Serial.print(ir2Detected ? "DETEKSI OBJEK (LOW)" : "BEBAS (HIGH)");
        Serial.print(" -> LPJ 2: ");
        Serial.println(ir2Detected ? "MENYALA (ON)" : "MATI (OFF)");

        lastStateIR1 = stateIR1;
        lastStateIR2 = stateIR2;
    }

    // Status detak jantung periodik setiap 500ms
    if (millis() - lastPrintTime >= PRINT_INTERVAL_MS) {
        lastPrintTime = millis();
        Serial.printf("[STATUS] IR1 (D34):%d | LPJ1:%s || IR2 (D35):%d | LPJ2:%s | IP:%s\n",
                      stateIR1,
                      ir1Detected ? "ON " : "OFF",
                      stateIR2,
                      ir2Detected ? "ON " : "OFF",
                      (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString().c_str() : "OFFLINE");
    }

    delay(20); // Debounce sampling singkat
}
