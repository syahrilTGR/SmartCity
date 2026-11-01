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

    // 4. Inisialisasi WiFiManager Non-Blocking & Portal AP 30 Detik
    wm.setConfigPortalBlocking(false); // NON-BLOCKING: Loop utama langsung jalan tanpa menunggu!
    wm.setConfigPortalTimeout(30);     // Portal AP aktif maks 30 detik di latar belakang
    wm.setConnectTimeout(4);           // Mencoba konek ke router tersimpan maks 4 detik

    Serial.println("\n[WIFI] Memulai WiFiManager (Mode Non-Blocking)...");
    Serial.println("[WIFI] Jika belum terhubung, sambungkan HP ke Wi-Fi: 'SmartCity-AP' (Aktif 30 detik)");

    if (wm.autoConnect("SmartCity-AP")) {
        Serial.println("[WIFI] Berhasil terhubung ke Wi-Fi!");
        Serial.print("[WIFI] Alamat IP ESP32: ");
        Serial.println(WiFi.localIP());
        setupOTA();
    } else {
        Serial.println("[WIFI] Portal 'SmartCity-AP' aktif di latar belakang (30 detik).");
        Serial.println("[WIFI] Loop utama diorama LANGSUNG BERJALAN tanpa jeda!");
    }

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

    delay(5); // Loop sampling interval ringan (200 FPS) untuk kestabilan RTOS
}