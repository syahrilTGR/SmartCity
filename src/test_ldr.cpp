/**
 * =============================================================================
 * PROGRAM PENGUJIAN: Sensor Cahaya LDR (Siang / Malam) & Lampu Rumah
 * + FITUR WIFI MANAGER & ARDUINO OTA (OVER-THE-AIR)
 * Proyek: Smart City Miniatur Diorama (ESP32 DevKit v1)
 * =============================================================================
 *
 * Alokasi Pin:
 * - Sensor LDR (VP)   : GPIO 36 (Input-Only, ADC1 Channel 0)
 *                       Rangkaian pembagi tegangan: 3.3V - LDR - GPIO 36 - R 10k - GND
 * - Lampu Rumah 1 & 2 : GPIO 18 (Output, LED Rumah via Resistor 47-51 Ohm)
 * - Lampu Rumah 3 & 4 : GPIO 19 (Output, LED Rumah via Resistor 47-51 Ohm)
 * =============================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// Definisi Pin
constexpr uint8_t PIN_LDR         = 36; // VP pin (Input-Only ADC1_CH0)
constexpr uint8_t PIN_HOUSE_1_2   = 18; // Output Lampu Rumah 1 & 2
constexpr uint8_t PIN_HOUSE_3_4   = 19; // Output Lampu Rumah 3 & 4

// Ambang Batas Default Siang/Malam (Dapat disesuaikan berdasarkan pencahayaan ruangan)
constexpr int LDR_THRESHOLD = 3650;

// Logging periodik
unsigned long lastPrintTime = 0;
constexpr unsigned long PRINT_INTERVAL_MS = 500;

// Status terakhir kondisi
bool lastIsNight = false;

void setHouseLights(bool state) {
    digitalWrite(PIN_HOUSE_1_2, state ? HIGH : LOW);
    digitalWrite(PIN_HOUSE_3_4, state ? HIGH : LOW);
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Konfigurasi Pin
    pinMode(PIN_LDR, INPUT);
    pinMode(PIN_HOUSE_1_2, OUTPUT);
    pinMode(PIN_HOUSE_3_4, OUTPUT);

    // Kondisi awal lampu padam
    setHouseLights(false);

    Serial.println("\n========================================================");
    Serial.println("   PENGUJIAN SENSOR LDR (SIANG / MALAM) & LAMPU RUMAH   ");
    Serial.println("        DILENGKAPI WIFIMANAGER & ARDUINO OTA            ");
    Serial.println("========================================================");

    // -------------------------------------------------------------------------
    // INISIALISASI WIFIMANAGER & ARDUINO OTA
    // -------------------------------------------------------------------------
    WiFiManager wm;
    wm.setConfigPortalTimeout(180);

    Serial.println("[WIFI] Memulai koneksi via WiFiManager...");
    bool isConnected = wm.autoConnect("SmartCity-LDR-AP");

    if (!isConnected) {
        Serial.println("[WIFI] Timeout portal tercapai / Gagal konek. Mode Offline aktif!");
    } else {
        Serial.println("[WIFI] Berhasil terhubung ke Wi-Fi!");
        Serial.print("[WIFI] Alamat IP: ");
        Serial.println(WiFi.localIP());

        ArduinoOTA.setHostname("smartcity-ldr");

        ArduinoOTA.onStart([]() {
            String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
            Serial.println("\n[OTA] Memulai transfer firmware Over-The-Air: " + type);
            setHouseLights(false);
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
        Serial.printf("    pio run -e test_ldr -t upload --upload-port %s\n", WiFi.localIP().toString().c_str());
        Serial.println("--------------------------------------------------------");
    }

    Serial.println("Mulai pengujian sensor LDR...\n");
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

    // Baca nilai ADC mentah (rentang 12-bit: 0 s/d 4095)
    int rawADC = analogRead(PIN_LDR);

    // Hitung perkiraan tegangan (0 - 3.3 Volt)
    float voltage = (rawADC / 4095.0f) * 3.3f;

    // Tentukan status malam vs siang
    bool isNight = (rawADC < LDR_THRESHOLD);

    // Kontrol Lampu Rumah
    setHouseLights(isNight);

    // Notifikasi jika terjadi pergantian status
    if (isNight != lastIsNight) {
        lastIsNight = isNight;
        Serial.println("\n----------------- [EVENT PERUBAHAN CAHAYA] -----------------");
        if (isNight) {
            Serial.printf(">>> KONDISI: MALAM HARI (ADC: %d < %d) -> LAMPU RUMAH MENYALA (ON) <<<\n", rawADC, LDR_THRESHOLD);
        } else {
            Serial.printf(">>> KONDISI: SIANG HARI (ADC: %d >= %d) -> LAMPU RUMAH PADAM (OFF) <<<\n", rawADC, LDR_THRESHOLD);
        }
        Serial.println("------------------------------------------------------------\n");
    }

    // Tampilkan data periodik ke Serial Monitor
    if (now - lastPrintTime >= PRINT_INTERVAL_MS) {
        lastPrintTime = now;

        // Visual progress bar nilai ADC sederhana
        int barLength = map(rawADC, 0, 4095, 0, 20);
        char bar[25];
        for (int i = 0; i < 20; i++) {
            bar[i] = (i < barLength) ? '#' : '.';
        }
        bar[20] = '\0';

        Serial.printf("[LDR] ADC:%4d | Volt:%4.2fV | Bar:[%s] | Suasana:%-5s | Rumah:%-3s | IP:%s\n",
                      rawADC,
                      voltage,
                      bar,
                      isNight ? "MALAM" : "SIANG",
                      isNight ? "ON " : "OFF",
                      (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString().c_str() : "OFFLINE");
    }

    delay(20);
}
