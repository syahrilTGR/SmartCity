/**
 * =============================================================================
 * FIRMWARE UTAMA: SMART CITY MINIATUR DIORAMA TERINTEGRASI
 * Mikrokontroler : ESP32 Dev Module (DOIT DevKit v1 30-Pin)
 * Framework      : Arduino (PlatformIO)
 * Fitur Tambahan : Software Debounce Digital & Histeresis Analog (Anti-Flicker)
 * =============================================================================
 *
 * ARSITEKTUR & SKENARIO SISTEM (Dengan Master Switch Otomasi SW SENS di GPIO 15):
 * 1. Penerangan Pemukiman (4 LED Rumah):
 *    - GPIO 18 (Rumah 1 & 2) & GPIO 19 (Rumah 3 & 4)
 *    - Menyala saat malam (LDR GPIO 36) jika saklar otomasi aktif (SW SENS = ON).
 *
 * 2. LPJ Jalur Atas (2 Tiang Lampu Otomatis):
 *    - GPIO 14 (LPJ Atas 1) dikendalikan IR 1 (GPIO 34).
 *    - GPIO 27 (LPJ Atas 2) dikendalikan IR 2 (GPIO 35).
 *    - Aktif jika SW SENS = ON (Debouncing + off-delay 1 detik).
 *
 * 3. LPJ Jalur Bawah (4 Tiang Lampu Simulator Rangkaian):
 *    - Master Enable: LDR (Malam) && Saklar Toggle (SW SENS = ON, GPIO 15 == LOW).
 *    - Seri Anti-Curang (GPIO 26): Wajib kabel BNC 1-2 (GPIO 4) & BNC 3-4 (GPIO 25) terpasang.
 *    - Paralel Mandiri (GPIO 33, 32): Dikendalikan cabang BNC 1 (GPIO 16) & cabang BNC 2 (GPIO 17).
 *    - Seluruh colokan BNC dan saklar dilengkapi filter debouncing 35 ms.
 *
 * 4. Lampu Lalu Lintas Cerdas Tenaga Surya (Traffic Light VKS 5V):
 *    - Sensor Surya: Optocoupler PC817 pada GPIO 39 (dengan histeresis anti-flicker).
 *    - Siang Hari (Daya Surya Aktif): Siklus Normal Merah -> Kuning -> Hijau.
 *    - Malam Hari (Daya Surya Padam): Mode Siaga Kota (Kuning Berkedip / Flashing Yellow).
 * =============================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// =============================================================================
// DEFINISI PINOUT (Sesuai docs/pinout.md & docs/pcb_wiring.md)
// =============================================================================

// Sensor Input-Only (ESP32 ADC1 / Digital Input)
constexpr uint8_t PIN_IR1         = 34; // IR Proximity 1 (Jalur Atas 1)
constexpr uint8_t PIN_IR2         = 35; // IR Proximity 2 (Jalur Atas 2)
constexpr uint8_t PIN_LDR         = 36; // Modul Sensor LDR DO (VP, Digital Input LM393)
constexpr uint8_t PIN_OPTO_SOLAR  = 39; // Optocoupler Panel Surya (VN, ADC1_CH3)

// Input Sensing Simulator & Saklar (Internal Pull-Up)
constexpr uint8_t PIN_BNC_SERI_1  = 4;  // Sensing BNC Seri Loop 1 (Lubang 1-2)
constexpr uint8_t PIN_BNC_SERI_2  = 25; // Sensing BNC Seri Loop 2 (Lubang 3-4, Anti-Curang)
constexpr uint8_t PIN_BNC_PAR_1   = 16; // Sensing BNC Paralel Cabang 1 (RX2)
constexpr uint8_t PIN_BNC_PAR_2   = 17; // Sensing BNC Paralel Cabang 2 (TX2)
constexpr uint8_t PIN_SW_SENS     = 15; // Saklar Toggle Master Otomasi Sensor

// Output Pencahayaan Miniatur
constexpr uint8_t PIN_LPJ_ATAS_1  = 14; // LPJ Jalur Atas 1 (Terpicu IR 1)
constexpr uint8_t PIN_LPJ_ATAS_2  = 27; // LPJ Jalur Atas 2 (Terpicu IR 2)
constexpr uint8_t PIN_LPJ_SERI    = 26; // LPJ Jalur Bawah Seri (2 LED diparalel fisik)
constexpr uint8_t PIN_LPJ_PAR_1   = 33; // LPJ Jalur Bawah Paralel 1 (Disesuaikan jalur solder)
constexpr uint8_t PIN_LPJ_PAR_2   = 32; // LPJ Jalur Bawah Paralel 2 (Disesuaikan jalur solder)

constexpr uint8_t PIN_HOUSE_1_2   = 18; // Lampu Rumah 1 & 2
constexpr uint8_t PIN_HOUSE_3_4   = 19; // Lampu Rumah 3 & 4

// Output Traffic Light VKS
constexpr uint8_t PIN_TL_RED      = 23; // Traffic Light Merah
constexpr uint8_t PIN_TL_YELLOW   = 5;  // Traffic Light Kuning
constexpr uint8_t PIN_TL_GREEN    = 13; // Traffic Light Hijau

// =============================================================================
// AMBANG BATAS (THRESHOLDS) & PARAMETER WAKTU
// =============================================================================

// Ambang Batas Histeresis Sensor LDR: Tidak dibutuhkan (Modul LM393 dikalibrasi via trimpot biru)

// Ambang Batas Histeresis Sensor Panel Surya PC817
constexpr int SOLAR_ENTER_DAY       = 2400; // Masuk daya surya aktif jika ADC < 2400
constexpr int SOLAR_EXIT_DAY        = 2600; // Kembali malam/padam jika ADC > 2600

// Parameter Debounce Digital (Milidetik)
constexpr unsigned long DEBOUNCE_DELAY_MS = 35;   // Filter getaran mekanik saklar/BNC
constexpr unsigned long IR_HOLD_TIME_MS   = 1000; // Off-delay lampu IR (tahan menyala 1s)

// Durasi Lampu Lalu Lintas Normal (Milidetik)
constexpr unsigned long DURATION_TL_RED    = 4000; // 4 detik
constexpr unsigned long DURATION_TL_YELLOW = 1500; // 1.5 detik
constexpr unsigned long DURATION_TL_GREEN  = 4000; // 4 detik

// Durasi Flashing Yellow Mode Siaga Malam (Milidetik)
constexpr unsigned long DURATION_BLINK     = 500;  // 500 ms ON, 500 ms OFF

// Interval Telemetri Serial Monitor
constexpr unsigned long TELEMETRY_INTERVAL = 600;

// =============================================================================
// KONFIGURASI MQTT BROKER & TOPIK TELEMETRI
// =============================================================================
constexpr const char* MQTT_BROKER       = "broker.emqx.io";
constexpr int         MQTT_PORT         = 1883;
constexpr const char* MQTT_TOPIC_STATUS = "smartcity/esp32/status";
constexpr const char* MQTT_TOPIC_TELEM  = "smartcity/esp32/telemetry";

constexpr unsigned long MQTT_RECONNECT_INTERVAL = 4000; // Interval coba koneksi ulang (ms)
constexpr unsigned long MQTT_PUBLISH_INTERVAL   = 1000; // Interval kirim data telemetri reguler (ms)
constexpr unsigned long MQTT_MIN_CHANGE_GAP     = 100;  // Batas jeda minimal pengiriman saat ada event perubahan (ms)

WiFiClient espClient;
PubSubClient mqttClient(espClient);

unsigned long lastMqttReconnectTime = 0;
unsigned long lastMqttPublishTime   = 0;

// =============================================================================
// STRUKTUR DATA DEBOUNCE UNTUK INPUT DIGITAL
// =============================================================================

struct DebouncedInput {
    uint8_t pin;
    int stableState;
    int lastReading;
    unsigned long lastDebounceTime;

    void init(uint8_t p, uint8_t mode) {
        pin = p;
        pinMode(pin, mode);
        stableState = digitalRead(pin);
        lastReading = stableState;
        lastDebounceTime = 0;
    }

    // Mengembalikan nilai stabil yang sudah ter-debounce
    int update(unsigned long now) {
        int reading = digitalRead(pin);

        if (reading != lastReading) {
            lastDebounceTime = now;
            lastReading = reading;
        }

        if ((now - lastDebounceTime) >= DEBOUNCE_DELAY_MS) {
            stableState = reading;
        }

        return stableState;
    }

    bool isLow(unsigned long now) {
        return (update(now) == LOW);
    }
};

// Instance Debounce untuk Setiap Input Mekanik & Sensor Digital
DebouncedInput btnBncSeri1;
DebouncedInput btnBncSeri2;
DebouncedInput btnBncPar1;
DebouncedInput btnBncPar2;
DebouncedInput btnToggleSens;
DebouncedInput sensorIR1;
DebouncedInput sensorIR2;

// Timer untuk Off-Delay Lampu LPJ Atas (menghindari lampu kedip saat bodi mobil lewat)
unsigned long ir1ActiveTimer = 0;
unsigned long ir2ActiveTimer = 0;

// Objek Global WiFiManager (untuk penanganan non-blocking portal di loop)
WiFiManager wm;

// =============================================================================
// STATE TRAFFIC LIGHT & TELEMETRI
// =============================================================================

enum TrafficLightState {
    TL_RED = 0,
    TL_YELLOW,
    TL_GREEN
};

TrafficLightState tlState = TL_RED;
unsigned long tlStateStartTime = 0;
unsigned long lastBlinkTime = 0;
bool yellowBlinkState = false;

unsigned long lastTelemetryTime = 0;

// Status Lingkungan
bool isNight = false;
bool isSolarDay = false;
bool isMasterSensorEnabled = false;
bool isLowerStreetArmed = false;
int currentLdrState = 0;    // 0 = LOW (Siang), 1 = HIGH (Malam dari DO LM393)
int currentRawSolar = 4095;

// =============================================================================
// HELPER FUNCTIONS
// =============================================================================

void setTrafficLight(bool r, bool y, bool g) {
    digitalWrite(PIN_TL_RED,    r ? HIGH : LOW);
    digitalWrite(PIN_TL_YELLOW, y ? HIGH : LOW);
    digitalWrite(PIN_TL_GREEN,  g ? HIGH : LOW);
}

// =============================================================================
// HELPER ADC MULTI-SAMPLE AVERAGING (STABILISASI NON-LINEARITAS & NOISE)
// =============================================================================

// Pembacaan ADC dengan 64 multi-sample averaging untuk menstabilkan output & meredam noise RF
int readSmoothADC(uint8_t pin, int samples = 64) {
    long sum = 0;
    for (int i = 0; i < samples; i++) {
        sum += analogRead(pin);
        delayMicroseconds(25);
    }
    return (int)(sum / samples);
}

// =============================================================================
// HELPER ARDUINO OTA (OVER-THE-AIR FIRMWARE UPDATE)
// =============================================================================

void setupOTA() {
    ArduinoOTA.setHostname("smartcity-esp32");

    ArduinoOTA.onStart([]() {
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        Serial.println("\n[OTA] Memulai transfer firmware Over-The-Air: " + type);
        // Matikan semua output lampu saat proses flash agar aman dari lonjakan arus
        digitalWrite(PIN_LPJ_ATAS_1, LOW);
        digitalWrite(PIN_LPJ_ATAS_2, LOW);
        digitalWrite(PIN_LPJ_SERI,   LOW);
        digitalWrite(PIN_LPJ_PAR_1,  LOW);
        digitalWrite(PIN_LPJ_PAR_2,  LOW);
        digitalWrite(PIN_HOUSE_1_2,  LOW);
        digitalWrite(PIN_HOUSE_3_4,  LOW);
        setTrafficLight(false, false, false);
    });

    ArduinoOTA.onEnd([]() {
        Serial.println("\n[OTA] Update Berhasil! ESP32 me-restart...");
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
    Serial.printf(">>> PERINTAH UPLOAD VIA WI-FI (OTA):\n");
    Serial.printf("    pio run -e esp32dev -t upload --upload-port %s\n", WiFi.localIP().toString().c_str());
}

// =============================================================================
// SETUP
// =============================================================================

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Konfigurasi atenuasi ADC skala penuh 0 - 3.3V (ADC_ATTEN_DB_11) untuk sensor panel surya
    analogSetAttenuation(ADC_11db);
    analogSetPinAttenuation(PIN_OPTO_SOLAR, ADC_11db);

    Serial.println();
    Serial.println("================================================================");
    Serial.println("     FIRMWARE UTAMA SMART CITY ESP32 (DEBOUNCED & HYSTERESIS)   ");
    Serial.println("================================================================");

    // 1. Inisialisasi Input Digital Ter-Debounce
    sensorIR1.init(PIN_IR1, INPUT);
    sensorIR2.init(PIN_IR2, INPUT);

    btnBncSeri1.init(PIN_BNC_SERI_1, INPUT_PULLUP);
    btnBncSeri2.init(PIN_BNC_SERI_2, INPUT_PULLUP);
    btnBncPar1.init(PIN_BNC_PAR_1, INPUT_PULLUP);
    btnBncPar2.init(PIN_BNC_PAR_2, INPUT_PULLUP);
    btnToggleSens.init(PIN_SW_SENS, INPUT_PULLUP);

    // 2. Inisialisasi Input Analog
    pinMode(PIN_LDR, INPUT);
    pinMode(PIN_OPTO_SOLAR, INPUT);

    // 3. Inisialisasi Output Lampu
    pinMode(PIN_LPJ_ATAS_1, OUTPUT);
    pinMode(PIN_LPJ_ATAS_2, OUTPUT);
    pinMode(PIN_LPJ_SERI,   OUTPUT);
    pinMode(PIN_LPJ_PAR_1,  OUTPUT);
    pinMode(PIN_LPJ_PAR_2,  OUTPUT);

    pinMode(PIN_HOUSE_1_2,  OUTPUT);
    pinMode(PIN_HOUSE_3_4,  OUTPUT);

    pinMode(PIN_TL_RED,     OUTPUT);
    pinMode(PIN_TL_YELLOW,  OUTPUT);
    pinMode(PIN_TL_GREEN,   OUTPUT);

    // Pastikan seluruh lampu awal padam
    digitalWrite(PIN_LPJ_ATAS_1, LOW);
    digitalWrite(PIN_LPJ_ATAS_2, LOW);
    digitalWrite(PIN_LPJ_SERI,   LOW);
    digitalWrite(PIN_LPJ_PAR_1,  LOW);
    digitalWrite(PIN_LPJ_PAR_2,  LOW);
    digitalWrite(PIN_HOUSE_1_2,  LOW);
    digitalWrite(PIN_HOUSE_3_4,  LOW);
    // Inisialisasi awal kondisi malam/siang & solar saat boot
    currentLdrState = digitalRead(PIN_LDR);
    isNight         = (currentLdrState == HIGH); // DO = HIGH (1) -> Malam
    currentRawSolar = readSmoothADC(PIN_OPTO_SOLAR);
    isSolarDay      = (currentRawSolar < SOLAR_ENTER_DAY);

    setTrafficLight(false, false, false);
    tlStateStartTime = millis();

    Serial.println("Inisialisasi I/O selesai. Seluruh filter debounce & histeresis aktif!");

    // 4. Inisialisasi WiFiManager Non-Blocking (Connect Timeout 15 Detik & Portal Standby Tanpa Batas)
    wm.setConfigPortalBlocking(false); // NON-BLOCKING: Loop utama langsung jalan tanpa menunggu!
    wm.setConfigPortalTimeout(0);      // Portal AP tetap terbuka (tanpa batas waktu) sampai ada yang menyambungkan Wi-Fi
    wm.setConnectTimeout(15);          // Coba konek ke router tersimpan selama 15 detik

    Serial.println("\n[WIFI] Memulai WiFiManager (Mode Non-Blocking)...");
    Serial.println("[WIFI] Mencoba konek ke router tersimpan (Maks 15 detik)...");

    if (wm.autoConnect("SmartCity-AP")) {
        Serial.println("[WIFI] Berhasil terhubung ke Wi-Fi!");
        Serial.print("[WIFI] Alamat IP ESP32: ");
        Serial.println(WiFi.localIP());
        setupOTA();
    } else {
        Serial.println("[WIFI] Belum terhubung dalam 15 detik. Portal 'SmartCity-AP' aktif di latar belakang (tanpa batas waktu).");
        Serial.println("[WIFI] Sambungkan HP ke 'SmartCity-AP' kapan saja untuk set Wi-Fi. Loop diorama LANGSUNG BERJALAN!");
    }

    // 5. Inisialisasi Klien MQTT (Buffer 1024 Bytes untuk JSON Telemetri)
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setBufferSize(1024);

    Serial.println("----------------------------------------------------------------");
}

// =============================================================================
// LOGIKA SUBSISTEM DENGAN FILTERING LENGKAP
// =============================================================================

/**
 * 1. Status Modul Sensor LDR (Siang / Malam):
 *    Membaca logika digital DO dari modul komparator LM393 (100% kebal interferensi Wi-Fi).
 *    - DO = HIGH (1) : Gelap / Malam Hari
 *    - DO = LOW  (0) : Terang / Siang Hari
 */
void updateLdrState(int ldrState) {
    isNight = (ldrState == HIGH);
}

/**
 * 2. Filter Histeresis untuk Panel Surya Optocoupler:
 *    Menghilangkan flicker pada traffic light saat cahaya senter goyang/remang.
 */
void updateSolarHysteresis(int rawSolar) {
    if (!isSolarDay && rawSolar < SOLAR_ENTER_DAY) {
        isSolarDay = true; // Beralih ke Daya Surya Aktif (Siang)
    } else if (isSolarDay && rawSolar > SOLAR_EXIT_DAY) {
        isSolarDay = false; // Beralih ke Tanpa Sinar Surya (Malam)
    }
}

/**
 * 3. Penerangan Rumah (4 LED):
 *    Menyala otomatis pada malam hari HANYA JIKA otomasi sensor aktif (SW SENS = ON).
 */
void updateHouseLights() {
    bool houseOn = (isMasterSensorEnabled && isNight);
    digitalWrite(PIN_HOUSE_1_2, houseOn ? HIGH : LOW);
    digitalWrite(PIN_HOUSE_3_4, houseOn ? HIGH : LOW);
}

/**
 * 4. LPJ Jalur Atas (2 LED):
 *    Otomasi dengan Debouncing + Off-Delay (Hold Time 1 Detik).
 *    Hanya aktif saat saklar otomasi sensor ON (SW SENS = ON).
 *    Jika SW SENS = OFF, lampu dipadamkan secara aman.
 */
void updateUpperStreetlights(unsigned long now) {
    if (!isMasterSensorEnabled) {
        digitalWrite(PIN_LPJ_ATAS_1, LOW);
        digitalWrite(PIN_LPJ_ATAS_2, LOW);
        ir1ActiveTimer = 0;
        ir2ActiveTimer = 0;
        return;
    }

    if (sensorIR1.isLow(now)) {
        ir1ActiveTimer = now;
        digitalWrite(PIN_LPJ_ATAS_1, HIGH);
    } else if (now - ir1ActiveTimer < IR_HOLD_TIME_MS) {
        digitalWrite(PIN_LPJ_ATAS_1, HIGH); // Tahan ON selama hold time
    } else {
        digitalWrite(PIN_LPJ_ATAS_1, LOW);  // Padam setelah hold time habis
    }

    if (sensorIR2.isLow(now)) {
        ir2ActiveTimer = now;
        digitalWrite(PIN_LPJ_ATAS_2, HIGH);
    } else if (now - ir2ActiveTimer < IR_HOLD_TIME_MS) {
        digitalWrite(PIN_LPJ_ATAS_2, HIGH);
    } else {
        digitalWrite(PIN_LPJ_ATAS_2, LOW);
    }
}

/**
 * 5. LPJ Jalur Bawah (4 LED Simulator):
 *    Menggunakan nilai BNC dan Saklar Toggle yang sudah ter-debounce 35ms.
 */
void updateLowerStreetlights(unsigned long now) {
    // Selalu perbarui pembacaan debounce seluruh kabel BNC
    bool bncSeri1 = btnBncSeri1.isLow(now);
    bool bncSeri2 = btnBncSeri2.isLow(now);
    bool bncPar1  = btnBncPar1.isLow(now);
    bool bncPar2  = btnBncPar2.isLow(now);

    if (isLowerStreetArmed) {
        // Logika Seri Anti-Curang (BNC 1-2 dan BNC 3-4 KEDUNYA harus terpasang)
        bool seriComplete = (bncSeri1 && bncSeri2);
        digitalWrite(PIN_LPJ_SERI, seriComplete ? HIGH : LOW);

        // Logika Paralel Mandiri
        digitalWrite(PIN_LPJ_PAR_1, bncPar1 ? HIGH : LOW);
        digitalWrite(PIN_LPJ_PAR_2, bncPar2 ? HIGH : LOW);
    } else {
        // Padamkan seluruh lampu jalan bawah jika siang atau saklar toggle OFF
        digitalWrite(PIN_LPJ_SERI,  LOW);
        digitalWrite(PIN_LPJ_PAR_1, LOW);
        digitalWrite(PIN_LPJ_PAR_2, LOW);
    }
}

/**
 * 6. Traffic Light VKS Tenaga Surya:
 *    - Siang (Solar Aktif): Siklus Normal Merah -> Kuning -> Hijau.
 *    - Malam (Solar Padam): Flashing Yellow (Mode Siaga).
 */
void updateTrafficLight(unsigned long now) {
    if (isSolarDay) {
        switch (tlState) {
            case TL_RED:
                setTrafficLight(true, false, false);
                if (now - tlStateStartTime >= DURATION_TL_RED) {
                    tlState = TL_YELLOW;
                    tlStateStartTime = now;
                }
                break;

            case TL_YELLOW:
                setTrafficLight(false, true, false);
                if (now - tlStateStartTime >= DURATION_TL_YELLOW) {
                    tlState = TL_GREEN;
                    tlStateStartTime = now;
                }
                break;

            case TL_GREEN:
                setTrafficLight(false, false, true);
                if (now - tlStateStartTime >= DURATION_TL_GREEN) {
                    tlState = TL_RED;
                    tlStateStartTime = now;
                }
                break;
        }
    } else {
        // Mode Malam / Tanpa Sinar Surya: Flashing Yellow Siaga
        if (now - lastBlinkTime >= DURATION_BLINK) {
            lastBlinkTime = now;
            yellowBlinkState = !yellowBlinkState;
            setTrafficLight(false, yellowBlinkState, false);
        }
    }
}

/**
 * 7. Telemetri Status Terpadu ke Serial Monitor (Non-Blocking)
 */
void printTelemetry(unsigned long now, int ldrState, int rawSolar) {
    if (now - lastTelemetryTime >= TELEMETRY_INTERVAL) {
        lastTelemetryTime = now;

        const char* tlStr = "SIAGA (Kuning Kedip)";
        if (isSolarDay) {
            if (tlState == TL_RED)         tlStr = "MERAH ";
            else if (tlState == TL_YELLOW) tlStr = "KUNING";
            else if (tlState == TL_GREEN)  tlStr = "HIJAU ";
        }

        Serial.printf("[CUACA] LDR_DO:%d (%-5s) | SOLAR:%4d (%-5s) | SW_SENS: %-3s || LPJ ATAS: [%d, %d] | LPJ BWH: [%s | P1:%d, P2:%d] | RUMAH: %-3s | TL: %-6s | IP: %s\n",
                      ldrState,
                      isNight ? "MALAM" : "SIANG",
                      rawSolar,
                      isSolarDay ? "SURYA" : "PADAM",
                      isMasterSensorEnabled ? "ON" : "OFF",
                      digitalRead(PIN_LPJ_ATAS_1) == HIGH ? 1 : 0,
                      digitalRead(PIN_LPJ_ATAS_2) == HIGH ? 1 : 0,
                      digitalRead(PIN_LPJ_SERI) == HIGH ? "SERI:ON " : "SERI:OFF",
                      digitalRead(PIN_LPJ_PAR_1) == HIGH ? 1 : 0,
                      digitalRead(PIN_LPJ_PAR_2) == HIGH ? 1 : 0,
                      digitalRead(PIN_HOUSE_1_2) == HIGH ? "ON " : "OFF",
                      tlStr,
                      (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString().c_str() : "OFFLINE");
    }
}

/**
 * 8. Publikasi Telemetri JSON ke MQTT Broker (Non-Blocking)
 */
void publishTelemetry(unsigned long now, bool force = false) {
    if (!mqttClient.connected()) return;

    if (!force && (now - lastMqttPublishTime < MQTT_PUBLISH_INTERVAL)) {
        return;
    }
    lastMqttPublishTime = now;

    JsonDocument doc;

    // 1. Kondisi Lingkungan & Cahaya
    doc["env"]["ldr_do"]       = currentLdrState;
    doc["env"]["night"]        = isNight;
    doc["env"]["solar_raw"]    = currentRawSolar;
    doc["env"]["solar_active"] = isSolarDay;
    doc["env"]["master_sens"]  = isMasterSensorEnabled;

    // 2. Traffic Light VKS
    if (isSolarDay) {
        doc["traffic"]["mode"] = "NORMAL";
        if (tlState == TL_RED)         doc["traffic"]["state"] = "RED";
        else if (tlState == TL_YELLOW) doc["traffic"]["state"] = "YELLOW";
        else if (tlState == TL_GREEN)  doc["traffic"]["state"] = "GREEN";
    } else {
        doc["traffic"]["mode"]  = "STANDBY_BLINK";
        doc["traffic"]["state"] = yellowBlinkState ? "BLINK" : "STANDBY";
    }

    // 3. LPJ Jalur Atas (2 Tiang Lampu IR 24/7)
    doc["street_upper"]["ir1"]  = sensorIR1.isLow(now);
    doc["street_upper"]["ir2"]  = sensorIR2.isLow(now);
    doc["street_upper"]["lpj1"] = (digitalRead(PIN_LPJ_ATAS_1) == HIGH);
    doc["street_upper"]["lpj2"] = (digitalRead(PIN_LPJ_ATAS_2) == HIGH);

    // 4. LPJ Jalur Bawah (Simulator Seri Anti-Curang & Paralel)
    doc["street_lower"]["armed"]     = isLowerStreetArmed;
    doc["street_lower"]["bnc_seri1"] = btnBncSeri1.isLow(now);
    doc["street_lower"]["bnc_seri2"] = btnBncSeri2.isLow(now);
    doc["street_lower"]["lpj_seri"]  = (digitalRead(PIN_LPJ_SERI) == HIGH);
    doc["street_lower"]["bnc_par1"]  = btnBncPar1.isLow(now);
    doc["street_lower"]["bnc_par2"]  = btnBncPar2.isLow(now);
    doc["street_lower"]["lpj_par1"]  = (digitalRead(PIN_LPJ_PAR_1) == HIGH);
    doc["street_lower"]["lpj_par2"]  = (digitalRead(PIN_LPJ_PAR_2) == HIGH);

    // 5. Penerangan Pemukiman (Rumah 1 - 4)
    doc["houses"]["house_1_2"] = (digitalRead(PIN_HOUSE_1_2) == HIGH);
    doc["houses"]["house_3_4"] = (digitalRead(PIN_HOUSE_3_4) == HIGH);

    // 6. Data Sistem
    doc["system"]["ip"]     = WiFi.localIP().toString();
    doc["system"]["rssi"]   = WiFi.RSSI();
    doc["system"]["uptime"] = now / 1000;

    char buffer[768];
    size_t len = serializeJson(doc, buffer, sizeof(buffer));
    mqttClient.publish(MQTT_TOPIC_TELEM, buffer, len);
}

/**
 * 9. Pengendali Koneksi MQTT & LWT (Non-Blocking)
 */
void handleMQTT(unsigned long now) {
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }

    if (!mqttClient.connected()) {
        if (now - lastMqttReconnectTime >= MQTT_RECONNECT_INTERVAL) {
            lastMqttReconnectTime = now;
            Serial.printf("[MQTT] Menghubungkan ke broker: %s:%d...\n", MQTT_BROKER, MQTT_PORT);

            // Client ID unik ESP32 berdasarkan MAC address
            String clientId = "ESP32_SmartCity_" + WiFi.macAddress();
            clientId.replace(":", "");

            // LWT (Last Will & Testament): Jika ESP32 mati/terputus, broker publikasikan "offline" (retained)
            if (mqttClient.connect(clientId.c_str(), MQTT_TOPIC_STATUS, 1, true, "offline")) {
                Serial.println("[MQTT] Berhasil terhubung ke Broker EMQX!");
                // Publikasikan status online (retained)
                mqttClient.publish(MQTT_TOPIC_STATUS, "online", true);
                // Kirim telemetri snapshot pertama
                publishTelemetry(now, true);
            } else {
                Serial.printf("[MQTT] Gagal terhubung, rc=%d. Mencoba lagi dalam 4 detik...\n", mqttClient.state());
            }
        }
    } else {
        mqttClient.loop();

        // 1. Publikasi telemetri periodik (1 detik)
        publishTelemetry(now, false);

        // 2. Publikasi instan jika ada transisi output lampu/sensor (Fast Response)
        static int lastLpj1 = -1, lastLpj2 = -1, lastSeri = -1, lastP1 = -1, lastP2 = -1, lastH1 = -1;
        static int lastTl = -1, lastNight = -1, lastSwSens = -1;
        int curLpj1   = digitalRead(PIN_LPJ_ATAS_1);
        int curLpj2   = digitalRead(PIN_LPJ_ATAS_2);
        int curSeri   = digitalRead(PIN_LPJ_SERI);
        int curP1     = digitalRead(PIN_LPJ_PAR_1);
        int curP2     = digitalRead(PIN_LPJ_PAR_2);
        int curH1     = digitalRead(PIN_HOUSE_1_2);
        int curTl     = isSolarDay ? (int)tlState : (yellowBlinkState ? 99 : 98);
        int curSwSens = isMasterSensorEnabled ? 1 : 0;

        bool stateChanged = (curLpj1 != lastLpj1 || curLpj2 != lastLpj2 || curSeri != lastSeri ||
                             curP1 != lastP1 || curP2 != lastP2 || curH1 != lastH1 ||
                             curTl != lastTl || isNight != lastNight || curSwSens != lastSwSens);

        if (stateChanged && (now - lastMqttPublishTime >= MQTT_MIN_CHANGE_GAP)) {
            lastLpj1 = curLpj1; lastLpj2 = curLpj2; lastSeri = curSeri;
            lastP1 = curP1; lastP2 = curP2; lastH1 = curH1;
            lastTl = curTl; lastNight = isNight; lastSwSens = curSwSens;
            publishTelemetry(now, true);
        }
    }
}

// =============================================================================
// MAIN LOOP (Non-Blocking Real-Time Architecture)
// =============================================================================

void loop() {
    unsigned long now = millis();

    // 0. Proses WiFiManager di latar belakang (Non-Blocking)
    wm.process();

    // Inisialisasi service OTA otomatis jika Wi-Fi terhubung belakangan via portal
    static bool otaReady = false;
    if (WiFi.status() == WL_CONNECTED) {
        if (!otaReady) {
            setupOTA();
            otaReady = true;
        }
        ArduinoOTA.handle();
    } else {
        otaReady = false;
    }

    // Reset Wi-Fi via Serial Monitor (Ketik huruf 'r' lalu Enter untuk ganti router)
    if (Serial.available()) {
        char c = Serial.read();
        if (c == 'r' || c == 'R') {
            Serial.println("\n[WIFI] Menghapus kredensial Wi-Fi tersimpan dan merestart ke mode Portal AP...");
            WiFiManager wm;
            wm.resetSettings();
            delay(1000);
            ESP.restart();
        }
    }

    // 1. Sampling Sensor: Modul LDR Digital & Sensor Panel Surya (ADC Optocoupler)
    static unsigned long lastSampleTime = 0;
    if (now - lastSampleTime >= 30) {
        lastSampleTime = now;
        currentLdrState = digitalRead(PIN_LDR);
        currentRawSolar = readSmoothADC(PIN_OPTO_SOLAR, 32);

        updateLdrState(currentLdrState);
        updateSolarHysteresis(currentRawSolar);
    }

    // 2. Sampling Saklar Toggle Master (Ter-Debounce)
    isMasterSensorEnabled = btnToggleSens.isLow(now);

    // Status Siap untuk LPJ Bawah
    isLowerStreetArmed = (isNight && isMasterSensorEnabled);

    // 3. Eksekusi Seluruh Subsistem Diorama
    updateHouseLights();
    updateUpperStreetlights(now);
    updateLowerStreetlights(now);
    updateTrafficLight(now);

    // 4. Pelaporan Status ke Serial Monitor
    printTelemetry(now, currentLdrState, currentRawSolar);

    // 5. Penanganan Komunikasi MQTT & Publikasi Telemetri (Non-Blocking)
    handleMQTT(now);

    delay(5); // Loop sampling interval ringan (200 FPS) untuk kestabilan RTOS
}