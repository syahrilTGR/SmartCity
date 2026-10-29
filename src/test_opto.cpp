#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

/**
 * ============================================================================
 * PROGRAM PENGUJIAN: Rangkaian Optocoupler PC817 & Traffic Light Bergantian
 *                    + FITUR WIFI MANAGER & ARDUINO OTA (OVER-THE-AIR)
 * Proyek: Smart City Miniatur Diorama (ESP32 DevKit v1)
 * ============================================================================
 * 
 * Pin Yang Diuji:
 * - GPIO 39 (VN) : Input sinyal dari Optocoupler PC817 (Collector)
 *                  Memerlukan Resistor Eksternal Pull-up 10k Ohm ke 3.3V.
 * 
 * Pin Traffic Light VKS:
 * - GPIO 23 : LED Merah   (R)
 * - GPIO 5  : LED Kuning  (Y)
 * - GPIO 13 : LED Hijau   (G)
 * 
 * Skenario Pengujian:
 * 1. WiFiManager:
 *    - Saat pertama kali nyala / Wi-Fi tidak ditemukan, ESP32 membuat Access Point:
 *      SSID: 'SmartCity-Opto-AP' (Tanpa Password).
 *    - Hubungkan HP/Laptop ke Wi-Fi tersebut, buka captive portal pop-up, dan
 *      pilih Wi-Fi serta masukkan password.
 * 
 * 2. ArduinoOTA:
 *    - Setelah terhubung ke Wi-Fi, upload firmware berikutnya bisa lewat jaringan:
 *      pio run -e test_opto -t upload --upload-port <IP_ADDRESS>
 * 
 * 3. Traffic Light:
 *    - Sinar Surya Aktif (ADC < 2500): Siklus Merah -> Kuning -> Hijau.
 *    - Gelap / Tanpa Sinar (ADC >= 2500): Siklus dihentikan (Lampu Padam).
 * 
 * 4. Perintah Serial Tambahan:
 *    - Ketik huruf 'r' di Serial Monitor untuk mereset kredensial Wi-Fi (ganti router).
 * ============================================================================
 */

// Definisi Pin
constexpr uint8_t PIN_OPTO_SOLAR = 39;  // VN pin (Input-Only ADC1_CH3)
constexpr uint8_t PIN_TL_RED     = 23;  // LED Merah Traffic Light
constexpr uint8_t PIN_TL_YELLOW  = 5;   // LED Kuning Traffic Light
constexpr uint8_t PIN_TL_GREEN   = 13;  // LED Hijau Traffic Light

// Batas Ambang Sinar Surya (Threshold ADC)
constexpr int ADC_THRESHOLD_SOLAR = 2500;

// Definisi State Traffic Light
enum TrafficLightState {
    TL_OFF = 0,
    TL_RED,
    TL_YELLOW,
    TL_GREEN
};

TrafficLightState currentState = TL_OFF;
unsigned long stateStartTime = 0;

// Durasi Masing-Masing Lampu (milidetik)
constexpr unsigned long DURATION_RED    = 2000;
constexpr unsigned long DURATION_YELLOW = 1000;
constexpr unsigned long DURATION_GREEN  = 2000;

// Logging periodik
unsigned long lastPrintTime = 0;
constexpr unsigned long PRINT_INTERVAL_MS = 500;

void setTrafficLight(bool r, bool y, bool g) {
    digitalWrite(PIN_TL_RED,    r ? HIGH : LOW);
    digitalWrite(PIN_TL_YELLOW, y ? HIGH : LOW);
    digitalWrite(PIN_TL_GREEN,  g ? HIGH : LOW);
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Konfigurasi Pin
    pinMode(PIN_OPTO_SOLAR, INPUT);
    pinMode(PIN_TL_RED,     OUTPUT);
    pinMode(PIN_TL_YELLOW,  OUTPUT);
    pinMode(PIN_TL_GREEN,   OUTPUT);

    // Awal mati semua
    setTrafficLight(false, false, false);

    Serial.println("\n========================================================");
    Serial.println("   PENGUJIAN OPTOCOUPLER PC817 + SIKLUS TRAFFIC LIGHT   ");
    Serial.println("        DILENGKAPI WIFIMANAGER & ARDUINO OTA            ");
    Serial.println("========================================================");

    // -------------------------------------------------------------------------
    // INISIALISASI WIFIMANAGER
    // -------------------------------------------------------------------------
    WiFiManager wm;

    // Timeout portal AP selama 180 detik (3 menit). Jika tidak ada koneksi baru,
    // ESP32 akan tetap lanjut menjalankan sistem secara offline.
    wm.setConfigPortalTimeout(180);

    Serial.println("[WIFI] Memulai koneksi via WiFiManager...");
    Serial.println("[WIFI] Jika belum terhubung ke router, hubungkan HP ke Wi-Fi:");
    Serial.println("[WIFI] -> SSID: 'SmartCity-Opto-AP' (Tanpa Password)");
    Serial.println("[WIFI] -> Buka browser/portal pop-up untuk setting Wi-Fi.");

    bool isConnected = wm.autoConnect("SmartCity-Opto-AP");

    if (!isConnected) {
        Serial.println("[WIFI] Timeout portal tercapai / Gagal konek. Mode Offline aktif!");
    } else {
        Serial.println("[WIFI] Berhasil terhubung ke Wi-Fi!");
        Serial.print("[WIFI] Alamat IP: ");
        Serial.println(WiFi.localIP());

        // ---------------------------------------------------------------------
        // INISIALISASI ARDUINO OTA (OVER-THE-AIR)
        // ---------------------------------------------------------------------
        ArduinoOTA.setHostname("smartcity-opto");

        ArduinoOTA.onStart([]() {
            String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
            Serial.println("\n[OTA] Memulai transfer firmware Over-The-Air: " + type);
            // Padamkan lampu saat proses flash agar aman dari lonjakan arus
            setTrafficLight(false, false, false);
        });

        ArduinoOTA.onEnd([]() {
            Serial.println("\n[OTA] Transfer Selesai! Me-restart ESP32...");
        });

        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            Serial.printf("[OTA] Progress Transfer: %u%%\r", (progress / (total / 100)));
        });

        ArduinoOTA.onError([](ota_error_t error) {
            Serial.printf("[OTA] Error [%u]: ", error);
            if (error == OTA_AUTH_ERROR)         Serial.println("Otorisasi Gagal");
            else if (error == OTA_BEGIN_ERROR)   Serial.println("Inisialisasi Gagal");
            else if (error == OTA_CONNECT_ERROR) Serial.println("Koneksi Gagal");
            else if (error == OTA_RECEIVE_ERROR) Serial.println("Penerimaan Gagal");
            else if (error == OTA_END_ERROR)     Serial.println("Finalisasi Gagal");
        });

        ArduinoOTA.begin();
        Serial.println("[OTA] Service ArduinoOTA Siap!");
        Serial.println("--------------------------------------------------------");
        Serial.printf(">>> PERINTAH UPLOAD VIA WI-FI (OTA):\n");
        Serial.printf("    pio run -e test_opto -t upload --upload-port %s\n", WiFi.localIP().toString().c_str());
        Serial.println("--------------------------------------------------------");
    }

    Serial.println("Sistem siap. Arahkan senter HP ke panel surya untuk mengaktifkan TL!\n");
}

void loop() {
    unsigned long now = millis();

    // 1. Handle ArduinoOTA jika terhubung ke Wi-Fi (Non-blocking)
    if (WiFi.status() == WL_CONNECTED) {
        ArduinoOTA.handle();
    }

    // 2. Fitur Reset Wi-Fi via Serial Monitor (Ketik huruf 'r' lalu Enter)
    if (Serial.available()) {
        char c = Serial.read();
        if (c == 'r' || c == 'R') {
            Serial.println("\n[WIFI] Menghapus kredensial Wi-Fi dan merestart ke mode AP Portal...");
            WiFiManager wm;
            wm.resetSettings();
            delay(1000);
            ESP.restart();
        }
    }

    // 3. Logika Pembacaan Optocoupler Panel Surya
    int rawAnalog = analogRead(PIN_OPTO_SOLAR); // Nilai 0 - 4095
    bool isSolarActive = (rawAnalog < ADC_THRESHOLD_SOLAR);

    if (isSolarActive) {
        // Jika baru pertama kali mendeteksi sinar surya
        if (currentState == TL_OFF) {
            Serial.println("\n>>> [SINAR AKTIF] Memulai siklus Traffic Light bergantian! <<<");
            currentState = TL_RED;
            stateStartTime = now;
            setTrafficLight(true, false, false);
        }

        // State Machine Siklus Bergantian Traffic Light (Non-Blocking)
        switch (currentState) {
            case TL_RED:
                if (now - stateStartTime >= DURATION_RED) {
                    currentState = TL_YELLOW;
                    stateStartTime = now;
                    setTrafficLight(false, true, false);
                    Serial.println(" -> TL Ganti: [KUNING]");
                }
                break;

            case TL_YELLOW:
                if (now - stateStartTime >= DURATION_YELLOW) {
                    currentState = TL_GREEN;
                    stateStartTime = now;
                    setTrafficLight(false, false, true);
                    Serial.println(" -> TL Ganti: [HIJAU]");
                }
                break;

            case TL_GREEN:
                if (now - stateStartTime >= DURATION_GREEN) {
                    currentState = TL_RED;
                    stateStartTime = now;
                    setTrafficLight(true, false, false);
                    Serial.println(" -> TL Ganti: [MERAH]");
                }
                break;

            case TL_OFF:
            default:
                break;
        }
    } else {
        // Jika sinar surya tidak terdeteksi (Gelap / tertutup)
        if (currentState != TL_OFF) {
            Serial.println("\n>>> [SINAR HILANG] Panel Gelap (ADC >= 2500) -> Traffic Light PADAM <<<");
            currentState = TL_OFF;
            setTrafficLight(false, false, false);
        }
    }

    // 4. Monitoring Serial periodik
    if (now - lastPrintTime >= PRINT_INTERVAL_MS) {
        lastPrintTime = now;
        const char* stateStr = "OFF";
        if (currentState == TL_RED) stateStr = "MERAH (ON)";
        else if (currentState == TL_YELLOW) stateStr = "KUNING (ON)";
        else if (currentState == TL_GREEN) stateStr = "HIJAU (ON)";

        Serial.printf("[SOLAR] ADC:%4d | Status:%-12s | TL:%-11s | IP:%s\n",
                      rawAnalog,
                      isSolarActive ? "TERANG" : "GELAP",
                      stateStr,
                      (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString().c_str() : "OFFLINE/AP");
    }

    delay(10); // Loop interval ringan untuk kestabilan RTOS
}
