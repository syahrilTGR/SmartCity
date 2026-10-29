/**
 * =============================================================================
 * Program Pengujian Simulator BNC Seri & Paralel + Lampu LPJ Jalur Bawah
 * + FITUR WIFI MANAGER & ARDUINO OTA (OVER-THE-AIR)
 * Proyek: Smart City Miniatur Diorama (ESP32 DevKit v1)
 * =============================================================================
 *
 * Alokasi Pin:
 * 1. SIMULATOR BNC SERI (Sistem Anti-Curang 2 Pasangan):
 *    - Sensing Loop 1   : GPIO 4  (INPUT_PULLUP) -> Lubang BNC 1-2
 *    - Sensing Loop 2   : GPIO 25 (INPUT_PULLUP) -> Lubang BNC 3-4
 *    - Output Lampu Seri: GPIO 26 (OUTPUT)       -> 2 LED Tiang LPJ Kiri Bawah
 *    * Logika: Lampu Seri (GPIO 26) ON jika dan HANYA JIKA kedua kabel BNC
 *      terpasang (GPIO 4 == LOW DAN GPIO 25 == LOW).
 *      Jika salah satu dicabut -> Lampu Seri OFF (kedua tiang mati).
 *
 * 2. SIMULATOR BNC PARALEL (2 Cabang Independen):
 *    - Sensing Cabang 1 : GPIO 16 / RX2 (INPUT_PULLUP) -> Lubang BNC Par 1
 *    - Sensing Cabang 2 : GPIO 17 / TX2 (INPUT_PULLUP) -> Lubang BNC Par 2
 *    - Output Lampu 1   : GPIO 33       (OUTPUT)       -> LPJ Kanan Bawah 1
 *    - Output Lampu 2   : GPIO 32       (OUTPUT)       -> LPJ Kanan Bawah 2
 *    * Logika: Sifat paralel mandiri.
 *      Cabang 1 terpasang (GPIO 16 == LOW) -> Lampu 1 (GPIO 33) ON.
 *      Cabang 2 terpasang (GPIO 17 == LOW) -> Lampu 2 (GPIO 32) ON.
 * =============================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// Definisi Pin Sensing BNC Seri
constexpr uint8_t PIN_BNC_SERI_1 = 4;
constexpr uint8_t PIN_BNC_SERI_2 = 25;
constexpr uint8_t PIN_LPJ_SERI   = 26; // 2 LED paralel fisik

// Definisi Pin Sensing BNC Paralel
constexpr uint8_t PIN_BNC_PAR_1  = 16;
constexpr uint8_t PIN_BNC_PAR_2  = 17;
constexpr uint8_t PIN_LPJ_PAR_1  = 33; // Disesuaikan jalur solder
constexpr uint8_t PIN_LPJ_PAR_2  = 32; // Disesuaikan jalur solder

// Variabel status sebelumnya untuk deteksi trigger
int lastSeri1 = -1, lastSeri2 = -1;
int lastPar1  = -1, lastPar2  = -1;

unsigned long lastPrintTime = 0;
constexpr unsigned long PRINT_INTERVAL_MS = 500;

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("==========================================================");
    Serial.println("   TESTING SIMULATOR BNC SERI & PARALEL + LPJ BAWAH      ");
    Serial.println("        DILENGKAPI WIFIMANAGER & ARDUINO OTA             ");
    Serial.println("==========================================================");

    // Konfigurasi Input dengan Resistor Pull-Up Internal ESP32
    pinMode(PIN_BNC_SERI_1, INPUT_PULLUP);
    pinMode(PIN_BNC_SERI_2, INPUT_PULLUP);
    pinMode(PIN_BNC_PAR_1,  INPUT_PULLUP);
    pinMode(PIN_BNC_PAR_2,  INPUT_PULLUP);

    // Konfigurasi Output Lampu LPJ
    pinMode(PIN_LPJ_SERI,  OUTPUT);
    pinMode(PIN_LPJ_PAR_1, OUTPUT);
    pinMode(PIN_LPJ_PAR_2, OUTPUT);

    // Pastikan seluruh lampu awal padam
    digitalWrite(PIN_LPJ_SERI,  LOW);
    digitalWrite(PIN_LPJ_PAR_1, LOW);
    digitalWrite(PIN_LPJ_PAR_2, LOW);

    // -------------------------------------------------------------------------
    // INISIALISASI WIFIMANAGER & ARDUINO OTA
    // -------------------------------------------------------------------------
    WiFiManager wm;
    wm.setConfigPortalTimeout(180);

    Serial.println("[WIFI] Memulai koneksi via WiFiManager...");
    bool isConnected = wm.autoConnect("SmartCity-BNC-AP");

    if (!isConnected) {
        Serial.println("[WIFI] Timeout portal tercapai / Gagal konek. Mode Offline aktif!");
    } else {
        Serial.println("[WIFI] Berhasil terhubung ke Wi-Fi!");
        Serial.print("[WIFI] Alamat IP: ");
        Serial.println(WiFi.localIP());

        ArduinoOTA.setHostname("smartcity-bnc");

        ArduinoOTA.onStart([]() {
            String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
            Serial.println("\n[OTA] Memulai transfer firmware Over-The-Air: " + type);
            digitalWrite(PIN_LPJ_SERI,  LOW);
            digitalWrite(PIN_LPJ_PAR_1, LOW);
            digitalWrite(PIN_LPJ_PAR_2, LOW);
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
        Serial.printf("    pio run -e test_bnc -t upload --upload-port %s\n", WiFi.localIP().toString().c_str());
        Serial.println("----------------------------------------------------------");
    }

    Serial.println("Mulai pengujian... Silakan colok/cabut kabel simulator BNC!\n");
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

    // 1. Baca Sinyal BNC Seri (LOW = Kabel terpasang ke GND)
    int seri1 = digitalRead(PIN_BNC_SERI_1);
    int seri2 = digitalRead(PIN_BNC_SERI_2);

    // Logika Anti-Curang Seri: Keduanya WAJIB terpasang
    bool seriComplete = (seri1 == LOW && seri2 == LOW);
    digitalWrite(PIN_LPJ_SERI, seriComplete ? HIGH : LOW);

    // 2. Baca Sinyal BNC Paralel (LOW = Kabel terpasang ke GND)
    int par1 = digitalRead(PIN_BNC_PAR_1);
    int par2 = digitalRead(PIN_BNC_PAR_2);

    // Logika Paralel: Masing-masing cabang mengendalikan lampunya sendiri
    bool par1Active = (par1 == LOW);
    bool par2Active = (par2 == LOW);
    digitalWrite(PIN_LPJ_PAR_1, par1Active ? HIGH : LOW);
    digitalWrite(PIN_LPJ_PAR_2, par2Active ? HIGH : LOW);

    // Cetak log hanya jika terjadi perubahan status fisik colokan BNC
    if (seri1 != lastSeri1 || seri2 != lastSeri2 || par1 != lastPar1 || par2 != lastPar2) {
        Serial.println("-------------------- [EVENT PERUBAHAN] --------------------");
        // Log Seri
        Serial.printf("[SERI] Loop1 (D4): %s | Loop2 (D25): %s => Lampu Seri (D26): %s\n",
                      (seri1 == LOW) ? "TERPASANG" : "TERPUTUS ",
                      (seri2 == LOW) ? "TERPASANG" : "TERPUTUS ",
                      seriComplete   ? "*** ON (MENYALA) ***" : "OFF (PADAM)");

        // Log Paralel
        Serial.printf("[PAR1] Cabang1 (D16): %s => Lampu Par 1 (D33): %s\n",
                      par1Active ? "TERPASANG" : "TERPUTUS ",
                      par1Active ? "ON " : "OFF");
        Serial.printf("[PAR2] Cabang2 (D17): %s => Lampu Par 2 (D32): %s\n",
                      par2Active ? "TERPASANG" : "TERPUTUS ",
                      par2Active ? "ON " : "OFF");
        Serial.println();

        lastSeri1 = seri1;
        lastSeri2 = seri2;
        lastPar1  = par1;
        lastPar2  = par2;
    }

    // Status detak jantung periodik setiap 500ms
    if (millis() - lastPrintTime >= PRINT_INTERVAL_MS) {
        lastPrintTime = millis();
        Serial.printf("[HEARTBEAT] SERI:%s (D4:%d, D25:%d) | P1:%s (D16:%d) | P2:%s (D17:%d) | IP:%s\n",
                      seriComplete ? "ON " : "OFF", seri1, seri2,
                      par1Active   ? "ON " : "OFF", par1,
                      par2Active   ? "ON " : "OFF", par2,
                      (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString().c_str() : "OFFLINE");
    }

    delay(20); // Debounce sampling
}
