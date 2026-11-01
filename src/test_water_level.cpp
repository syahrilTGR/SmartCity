/**
 * =============================================================================
 * SISTEM MONITORING KETINGGIAN AIR + KONTROL POMPA (TB6612FNG)
 * ESP32 + HC-SR04 + TB6612FNG + 3 BUTTON + SAKLAR ON/OFF SENSOR
 * + FITUR WIFIMANAGER & ARDUINO OTA (OVER-THE-AIR)
 * =============================================================================
 *
 * Alokasi Pin:
 * - Sensor Ultrasonic HC-SR04 : Trig = GPIO 5, Echo = GPIO 18
 * - Indikator LED Ketinggian   : Hijau = GPIO 2, Kuning = GPIO 4, Merah = GPIO 21
 * - Peringatan Suara           : Buzzer = GPIO 19
 * - Driver Motor TB6612FNG     : PWMA = GPIO 23, AIN1 = GPIO 25, AIN2 = GPIO 26, STBY = GPIO 27
 * - Tombol Pemilih Kecepatan   : Pelan = GPIO 32, Sedang = GPIO 33, Cepat = GPIO 14 (INPUT_PULLUP)
 * - Saklar Master Sensor       : GPIO 34 (HIGH = ON, LOW = OFF)
 *
 * Fitur Tambahan Terintegrasi:
 * 1. WiFiManager: Hotspot AP 'SmartCity-Water-AP' untuk setting Wi-Fi via HP tanpa hardcode.
 * 2. ArduinoOTA : Upload firmware baru langsung via Wi-Fi (Non-Blocking).
 * 3. Non-Blocking Loop: Menggantikan delay(1000) dengan millis() agar responsif terhadap OTA.
 * =============================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// ==========================================
// DEFINISI PIN
// ==========================================

// Pin HC-SR04
constexpr int trig_pin = 5;
constexpr int echo_pin = 18;

// Pin LED Indikator
constexpr int led_hijau  = 2;
constexpr int led_kuning = 4;
constexpr int led_merah  = 21;

// Pin Buzzer
constexpr int buzzer_pin = 19;

// Pin Driver TB6612FNG
constexpr int PWMA = 23;
constexpr int AIN1 = 25;
constexpr int AIN2 = 26;
constexpr int STBY = 27;

// Pin Tombol Kecepatan Pompa (Active LOW / Internal Pullup)
constexpr int button_pelan  = 32;
constexpr int button_sedang = 33;
constexpr int button_cepat  = 14;

// Pin Saklar Sensor (GPIO 34)
constexpr int sensorSwitch = 34;

// ==========================================
// PARAMETER HC-SR04
// ==========================================
#define SOUND_SPEED 340
#define TRIG_PULSE_DURATION_US 10

long ultrason_duration = 0;
float distance_cm = 0.0f;

// ==========================================
// KECEPATAN POMPA (PWM ESP32: 0 - 255)
// ==========================================
int pumpSpeed = 0;
const int SPEED_PELAN  = 77;   // ±30%
const int SPEED_SEDANG = 153;  // ±60%
const int SPEED_CEPAT  = 255;  // 100%

// ==========================================
// TIMING INTERVAL NON-BLOCKING (MILLIS)
// ==========================================
unsigned long lastSensorReadTime = 0;
constexpr unsigned long SENSOR_INTERVAL_MS = 600; // Baca jarak setiap 600ms

unsigned long lastButtonCheckTime = 0;
constexpr unsigned long BUTTON_DEBOUNCE_MS = 50;

// Fungsi helper mematikan seluruh aktuator & lampu
void turnOffAllOutputs() {
    digitalWrite(led_hijau, LOW);
    digitalWrite(led_kuning, LOW);
    digitalWrite(led_merah, LOW);
    digitalWrite(buzzer_pin, LOW);
    pumpSpeed = 0;
    analogWrite(PWMA, 0);
    digitalWrite(STBY, LOW); // Standby driver motor
}

// ==========================================
// SETUP
// ==========================================
void setup() {
    Serial.begin(115200);
    delay(1000);

    // -------------------------------
    // Inisialisasi HC-SR04
    // -------------------------------
    pinMode(trig_pin, OUTPUT);
    pinMode(echo_pin, INPUT);
    digitalWrite(trig_pin, LOW);

    // -------------------------------
    // Inisialisasi LED Indikator
    // -------------------------------
    pinMode(led_hijau, OUTPUT);
    pinMode(led_kuning, OUTPUT);
    pinMode(led_merah, OUTPUT);
    digitalWrite(led_hijau, LOW);
    digitalWrite(led_kuning, LOW);
    digitalWrite(led_merah, LOW);

    // -------------------------------
    // Inisialisasi Buzzer
    // -------------------------------
    pinMode(buzzer_pin, OUTPUT);
    digitalWrite(buzzer_pin, LOW);

    // -------------------------------
    // Inisialisasi TB6612FNG
    // -------------------------------
    pinMode(PWMA, OUTPUT);
    pinMode(AIN1, OUTPUT);
    pinMode(AIN2, OUTPUT);
    pinMode(STBY, OUTPUT);

    digitalWrite(STBY, HIGH);
    digitalWrite(AIN1, HIGH); // Arah putaran pompa
    digitalWrite(AIN2, LOW);
    analogWrite(PWMA, 0);     // Pompa awal OFF

    // -------------------------------
    // Inisialisasi Button & Saklar
    // -------------------------------
    pinMode(button_pelan, INPUT_PULLUP);
    pinMode(button_sedang, INPUT_PULLUP);
    pinMode(button_cepat, INPUT_PULLUP);
    pinMode(sensorSwitch, INPUT);

    Serial.println("\n========================================================");
    Serial.println("  SISTEM MONITORING KETINGGIAN AIR + POMPA OTOMASI     ");
    Serial.println("        DILENGKAPI WIFIMANAGER & ARDUINO OTA            ");
    Serial.println("========================================================");

    // -------------------------------------------------------------------------
    // INISIALISASI WIFIMANAGER
    // -------------------------------------------------------------------------
    WiFiManager wm;
    wm.setConfigPortalTimeout(180); // 3 menit timeout portal konfigurasi

    Serial.println("[WIFI] Memulai koneksi via WiFiManager...");
    Serial.println("[WIFI] Jika belum terhubung, sambungkan HP ke Wi-Fi:");
    Serial.println("[WIFI] -> SSID: 'SmartCity-Water-AP' (Tanpa Password)");
    Serial.println("[WIFI] -> Buka captive portal untuk memilih Wi-Fi & password.");

    bool isConnected = wm.autoConnect("SmartCity-Water-AP");

    if (!isConnected) {
        Serial.println("[WIFI] Timeout portal tercapai / Gagal konek. Melanjutkan mode offline!");
    } else {
        Serial.println("[WIFI] Berhasil terhubung ke Wi-Fi!");
        Serial.print("[WIFI] Alamat IP: ");
        Serial.println(WiFi.localIP());

        // ---------------------------------------------------------------------
        // INISIALISASI ARDUINO OTA (OVER-THE-AIR)
        // ---------------------------------------------------------------------
        ArduinoOTA.setHostname("smartcity-water");

        ArduinoOTA.onStart([]() {
            String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
            Serial.println("\n[OTA] Memulai transfer firmware Over-The-Air: " + type);
            // Matikan pompa, buzzer, dan LED demi keselamatan saat proses flash
            turnOffAllOutputs();
        });

        ArduinoOTA.onEnd([]() {
            Serial.println("\n[OTA] Transfer Selesai! Me-restart ESP32...");
        });

        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            Serial.printf("[OTA] Progress Transfer: %u%%\r", (progress / (total / 100)));
        });

        ArduinoOTA.onError([](ota_error_t error) {
            Serial.printf("[OTA] Error [%u]\n", error);
        });

        ArduinoOTA.begin();
        Serial.println("[OTA] Service ArduinoOTA Siap!");
        Serial.println("--------------------------------------------------------");
        Serial.printf(">>> PERINTAH UPLOAD VIA WI-FI (OTA):\n");
        Serial.printf("    pio run -e test_water -t upload --upload-port %s\n", WiFi.localIP().toString().c_str());
        Serial.println("--------------------------------------------------------");
    }

    Serial.println("Sistem monitoring air siap!\n");
}

// ==========================================
// LOOP (Non-Blocking Architecture)
// ==========================================
void loop() {
    unsigned long now = millis();

    // -------------------------------------------------------------------------
    // 0. HANDLE OTA & RESET WIFI (NON-BLOCKING)
    // -------------------------------------------------------------------------
    if (WiFi.status() == WL_CONNECTED) {
        ArduinoOTA.handle();
    }

    // Reset Wi-Fi via Serial Monitor (Ketik huruf 'r' lalu Enter untuk ganti router)
    if (Serial.available()) {
        char c = Serial.read();
        if (c == 'r' || c == 'R') {
            Serial.println("\n[WIFI] Mereset kredensial Wi-Fi tersimpan dan merestart...");
            WiFiManager wm;
            wm.resetSettings();
            delay(1000);
            ESP.restart();
        }
    }

    // -------------------------------------------------------------------------
    // 1. CEK STATUS SAKLAR MASTER SENSOR (GPIO 34)
    // -------------------------------------------------------------------------
    bool isSensorOn = (digitalRead(sensorSwitch) == HIGH);

    if (!isSensorOn) {
        // Mode SENSOR OFF: Matikan seluruh LED, Buzzer, dan Pompa
        turnOffAllOutputs();

        // Cetak status periodik setiap 1 detik saat saklar OFF
        if (now - lastSensorReadTime >= 1000) {
            lastSensorReadTime = now;
            Serial.printf("[STATUS] SAKLAR SENSOR: OFF | Pompa: OFF | IP: %s\n",
                          (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString().c_str() : "OFFLINE");
        }
        delay(10);
        return; // Lewati pembacaan sensor dan kontrol pompa
    }

    // Pastikan driver motor aktif jika sensor ON
    digitalWrite(STBY, HIGH);
    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);

    // -------------------------------------------------------------------------
    // 2. BACA TOMBOL KECEPATAN POMPA (DEBOUNCED)
    // -------------------------------------------------------------------------
    if (now - lastButtonCheckTime >= BUTTON_DEBOUNCE_MS) {
        lastButtonCheckTime = now;

        if (digitalRead(button_pelan) == LOW) {
            pumpSpeed = SPEED_PELAN;
            Serial.println(">>> Tombol: Pompa PELAN (30%) <<<");
        } else if (digitalRead(button_sedang) == LOW) {
            pumpSpeed = SPEED_SEDANG;
            Serial.println(">>> Tombol: Pompa SEDANG (60%) <<<");
        } else if (digitalRead(button_cepat) == LOW) {
            pumpSpeed = SPEED_CEPAT;
            Serial.println(">>> Tombol: Pompa CEPAT (100%) <<<");
        }

        // Terapkan kecepatan ke driver motor
        analogWrite(PWMA, pumpSpeed);
    }

    // -------------------------------------------------------------------------
    // 3. PEMBACAAN JARAK ULTRASONIC & INDIKATOR KETINGGIAN AIR
    // -------------------------------------------------------------------------
    if (now - lastSensorReadTime >= SENSOR_INTERVAL_MS) {
        lastSensorReadTime = now;

        // Trigger HC-SR04
        digitalWrite(trig_pin, LOW);
        delayMicroseconds(2);
        digitalWrite(trig_pin, HIGH);
        delayMicroseconds(TRIG_PULSE_DURATION_US);
        digitalWrite(trig_pin, LOW);

        // Baca pulse Echo (Timeout 30ms = max ~5 meter)
        ultrason_duration = pulseIn(echo_pin, HIGH, 30000);

        // Hitung jarak (cm)
        distance_cm = (ultrason_duration * SOUND_SPEED / 2.0f) * 0.0001f;

        const char* statusKetinggian = "UNKNOWN";

        // Logika Indikator Air
        if (ultrason_duration == 0) {
            // Sensor tidak mendapat pantulan
            digitalWrite(led_hijau,  LOW);
            digitalWrite(led_kuning, LOW);
            digitalWrite(led_merah,  LOW);
            digitalWrite(buzzer_pin, LOW);
            statusKetinggian = "TIDAK TERBACA";
        } else if (distance_cm > 10.0f) {
            // NORMAL - Air masih jauh dari bibir penampung
            digitalWrite(led_hijau,  HIGH);
            digitalWrite(led_kuning, LOW);
            digitalWrite(led_merah,  LOW);
            digitalWrite(buzzer_pin, LOW);
            statusKetinggian = "NORMAL (HIJAU)";
        } else if (distance_cm > 5.0f) {
            // WASPADA - Air mulai mendekat
            digitalWrite(led_hijau,  LOW);
            digitalWrite(led_kuning, HIGH);
            digitalWrite(led_merah,  LOW);
            digitalWrite(buzzer_pin, LOW);
            statusKetinggian = "WASPADA (KUNING)";
        } else {
            // BAHAYA - Air meluap / sangat tinggi
            digitalWrite(led_hijau,  LOW);
            digitalWrite(led_kuning, LOW);
            digitalWrite(led_merah,  HIGH);
            digitalWrite(buzzer_pin, HIGH);
            statusKetinggian = "BAHAYA (MERAH+BUZZER)";
        }

        // Tampilkan Telemetri
        Serial.printf("[WATER] Jarak: %5.1f cm | Status: %-21s | PWM Pompa: %3d | IP: %s\n",
                      distance_cm,
                      statusKetinggian,
                      pumpSpeed,
                      (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString().c_str() : "OFFLINE");
    }

    delay(5); // Loop interval sangat ringan (200 FPS) agar ArduinoOTA sangat responsif
}
