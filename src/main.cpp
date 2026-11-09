/**
 * =============================================================================
 * FIRMWARE UTAMA: SMART CITY MINIATUR DIORAMA TERINTEGRASI
 * Mikrokontroler : ESP32 Dev Module (DOIT DevKit v1 30-Pin)
 * Framework      : Arduino (PlatformIO)
 * Fitur Tambahan : Software Debounce Digital & Histeresis Analog (Anti-Flicker)
 * =============================================================================
 *
 * ARSITEKTUR & SKENARIO SISTEM (Revisi 10-Port BNC & Motor Driver L298N):
 * 1. Penerangan Pemukiman (4 LED Rumah):
 *    - GPIO 2 (Soket L): 4 LED Rumah menyala saat malam hari (LDR = HIGH / Gelap).
 *
 * 2. LPJ Jalur Atas (2 Tiang Lampu Otomatis):
 *    - GPIO 14 (LPJ Atas 1) dikendalikan IR 1 (GPIO 34).
 *    - GPIO 27 (LPJ Atas 2) dikendalikan IR 2 (GPIO 35).
 *    - Bersiaga HANYA saat malam hari (isNight == true). Siang hari dipadamkan aman.
 *
 * 3. LPJ Jalur Bawah (4 Tiang Lampu Mandiri Simulator BNC):
 *    - Lampu 1 (GPIO 26 / Soket H), Lampu 2 (GPIO 12 / Soket M).
 *    - Lampu 3 (GPIO 33 / Soket D Pin 3), Lampu 4 (GPIO 32 / Soket D Pin 1).
 *    - Aktif saat malam hari (isNight == true) sesuai koneksi kabel BNC siswa.
 *
 * 4. Motor Driver L298N (2x Dinamo DC 1-Arah):
 *    - GPIO 15 (Soket J): Output kendali motor IN1 + IN3 L298N (dapat diatur ON/OFF & PWM).
 *
 * 5. Master Switch Otomasi Sensor:
 *    - Saklar toggle fisik dipasang seri memutus jalur 3.3V ke Modul LDR (Soket I).
 *    - Saat OFF: LDR mati fisik -> terbaca Siang -> seluruh otomasi lampu padam seketika.
 *
 * 6. Lampu Lalu Lintas Cerdas Tenaga Surya (Traffic Light VKS 5V):
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

// =============================================================================
// DEFINISI 8 PORT BANANA / BNC (MAPPING AWAL 10-PORT)
// [O]: D4(A1), SRC+(3V3+1k), D25(A2), SRC-(GND)
// [N]: RX2/D16(B1), D21(C1), TX2/D17(B2)
// [K]: D18(D1), D19(D2), D22(C2)
// =============================================================================
enum BananaPortIndex {
    PORT_A1 = 0, // Indeks 0: GPIO 4  (Soket [O] Pin 1) -> Beban Lampu 1
    PORT_A2,     // Indeks 1: GPIO 25 (Soket [O] Pin 3) -> Beban Lampu 1
    PORT_B1,     // Indeks 2: GPIO 16 (Soket [N] Pin 1) -> Beban Lampu 2
    PORT_B2,     // Indeks 3: GPIO 17 (Soket [N] Pin 3) -> Beban Lampu 2
    PORT_C1,     // Indeks 4: GPIO 21 (Soket [N] Pin 2) -> Beban Lampu 3
    PORT_C2,     // Indeks 5: GPIO 22 (Soket [K] Pin 3) -> Beban Lampu 3
    PORT_D1,     // Indeks 6: GPIO 18 (Soket [K] Pin 1) -> Beban Lampu 4
    PORT_D2,     // Indeks 7: GPIO 19 (Soket [K] Pin 2) -> Beban Lampu 4
    PORT_COUNT = 8
};

// Pemetaan Pin GPIO ESP32 untuk 8 Port Banana (Urut Indeks 0..7)
constexpr uint8_t BANANA_PINS[PORT_COUNT] = {
    4,   // Index 0: PORT_A1 (D4  - Soket [O] Pin 1) -> Beban Lampu 1
    25,  // Index 1: PORT_A2 (D25 - Soket [O] Pin 3) -> Beban Lampu 1
    16,  // Index 2: PORT_B1 (RX2 - Soket [N] Pin 1) -> Beban Lampu 2
    17,  // Index 3: PORT_B2 (TX2 - Soket [N] Pin 3) -> Beban Lampu 2
    21,  // Index 4: PORT_C1 (D21 - Soket [N] Pin 2) -> Beban Lampu 3
    22,  // Index 5: PORT_C2 (D22 - Soket [K] Pin 3) -> Beban Lampu 3
    18,  // Index 6: PORT_D1 (D18 - Soket [K] Pin 1) -> Beban Lampu 4
    19   // Index 7: PORT_D2 (D19 - Soket [K] Pin 2) -> Beban Lampu 4
};

// Output Kendali Motor Driver L298N (2x Dinamo DC 1-Arah, Soket J)
constexpr uint8_t PIN_MOTOR_DRV   = 15; // Kendali Motor L298N IN1+IN3 (Eks SW_SENS)

// Output Pencahayaan Miniatur
constexpr uint8_t PIN_LPJ_ATAS_1  = 14; // LPJ Jalur Atas 1 (Terpicu IR 1 Malam Hari)
constexpr uint8_t PIN_LPJ_ATAS_2  = 27; // LPJ Jalur Atas 2 (Terpicu IR 2 Malam Hari)
constexpr uint8_t PIN_LPJ_BWH_1   = 26; // LPJ Bawah 1 (Soket H - D26) - Pasangan A1 & A2
constexpr uint8_t PIN_LPJ_BWH_2   = 12; // LPJ Bawah 2 (Soket M - D12) - Pasangan B1 & B2
constexpr uint8_t PIN_LPJ_BWH_3   = 33; // LPJ Bawah 3 (Soket D Pin 3 - D33) - Pasangan C1 & C2
constexpr uint8_t PIN_LPJ_BWH_4   = 32; // LPJ Bawah 4 (Soket D Pin 1 - D32) - Pasangan D1 & D2

// Array Output 4 Lampu Jalan Miniatur Bawah (Urut Lampu 1..4)
constexpr uint8_t LPJ_BWH_PINS[4] = {
    PIN_LPJ_BWH_1,
    PIN_LPJ_BWH_2,
    PIN_LPJ_BWH_3,
    PIN_LPJ_BWH_4
};

// Penerangan Pemukiman (4 LED Rumah digabung ke GPIO 2, Soket L)
constexpr uint8_t PIN_HOUSE       = 2;  // Seluruh Lampu Rumah 1-4

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

// Instance Debounce untuk Sensor Digital IR
DebouncedInput sensorIR1;
DebouncedInput sensorIR2;

// =============================================================================
// STRUKTUR DATA & STATE SIMULATOR 10-PORT BANANA (BEBAN A s/d D)
// =============================================================================
constexpr uint8_t NODE_COUNT = 10;
constexpr uint8_t NODE_VCC   = 8; // Indeks Node Sumber (+) 3.3V
constexpr uint8_t NODE_GND   = 9; // Indeks Node Sumber (-) Ground

struct DSU {
    uint8_t parent[NODE_COUNT];
    void init() {
        for (uint8_t i = 0; i < NODE_COUNT; i++) parent[i] = i;
    }
    uint8_t find(uint8_t x) {
        if (parent[x] == x) return x;
        return parent[x] = find(parent[x]);
    }
    void unite(uint8_t a, uint8_t b) {
        uint8_t rootA = find(a);
        uint8_t rootB = find(b);
        if (rootA != rootB) parent[rootB] = rootA;
    }
};

// Status Global Simulator Rangkaian
bool isSrcShortCircuited = false;       // Terdeteksi korslet langsung SRC+ ke SRC-
uint8_t lpjBwhDuty[4]    = {0, 0, 0, 0}; // Nilai PWM Kecerahan 0..255 untuk Lampu 1..4
uint8_t lpjBwhChain[4]   = {0, 0, 0, 0}; // Panjang rantai seri (1=Paralel, 2=2 Seri, dst)

// Status & Kendali Motor Driver L298N (GPIO 15)
bool isMotorActive = true; // Motor aktif berputar (dapat dikendalikan via MQTT / Serial)

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
        // Matikan semua output lampu & motor saat proses flash agar aman dari lonjakan arus
        digitalWrite(PIN_LPJ_ATAS_1, LOW);
        digitalWrite(PIN_LPJ_ATAS_2, LOW);
        for (uint8_t k = 0; k < 4; k++) {
            analogWrite(LPJ_BWH_PINS[k], 0);
        }
        digitalWrite(PIN_HOUSE,      LOW);
        digitalWrite(PIN_MOTOR_DRV,  LOW);
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

    // 1. Inisialisasi Input Digital Ter-Debounce & 8 Port Banana Simulator
    sensorIR1.init(PIN_IR1, INPUT);
    sensorIR2.init(PIN_IR2, INPUT);

    for (uint8_t i = 0; i < PORT_COUNT; i++) {
        pinMode(BANANA_PINS[i], INPUT_PULLUP);
    }

    // 2. Inisialisasi Input Analog
    pinMode(PIN_LDR, INPUT);
    pinMode(PIN_OPTO_SOLAR, INPUT);

    // 3. Inisialisasi Output Lampu & Motor
    pinMode(PIN_LPJ_ATAS_1, OUTPUT);
    pinMode(PIN_LPJ_ATAS_2, OUTPUT);
    for (uint8_t k = 0; k < 4; k++) {
        pinMode(LPJ_BWH_PINS[k], OUTPUT);
        analogWrite(LPJ_BWH_PINS[k], 0);
    }

    pinMode(PIN_HOUSE,      OUTPUT);
    pinMode(PIN_MOTOR_DRV,  OUTPUT);

    pinMode(PIN_TL_RED,     OUTPUT);
    pinMode(PIN_TL_YELLOW,  OUTPUT);
    pinMode(PIN_TL_GREEN,   OUTPUT);

    // Pastikan seluruh lampu & motor awal padam/diam
    digitalWrite(PIN_LPJ_ATAS_1, LOW);
    digitalWrite(PIN_LPJ_ATAS_2, LOW);
    digitalWrite(PIN_HOUSE,      LOW);
    digitalWrite(PIN_MOTOR_DRV,  LOW);
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
 *    Menyala otomatis pada malam hari (LDR = HIGH / Gelap).
 */
void updateHouseLights() {
    digitalWrite(PIN_HOUSE, isNight ? HIGH : LOW);
}

/**
 * 4. LPJ Jalur Atas (2 LED):
 *    Otomasi dengan Debouncing + Off-Delay (Hold Time 1 Detik).
 *    Hanya aktif bersiaga jika malam hari (isNight == true).
 *    Di siang hari, lampu PJU jalan raya dipadamkan aman.
 */
void updateUpperStreetlights(unsigned long now) {
    if (!isNight) {
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
 * Pencarian Rekursif Graf Rangkaian (DFS)
 * Menelusuri seluruh jalur sederhana dari currentNet ke targetNet melalui lampu-lampu diorama.
 */
void searchCircuitPaths(uint8_t currentNet, uint8_t targetNet, uint8_t visitedLamps, uint8_t currentLength, 
                        uint8_t netOfLamp[4][2], uint8_t minChain[4]) {
    if (currentNet == targetNet) {
        // Sirkuit tertutup dari VCC ke GND berhasil terbentuk!
        for (uint8_t k = 0; k < 4; k++) {
            if (visitedLamps & (1 << k)) {
                if (currentLength < minChain[k]) {
                    minChain[k] = currentLength;
                }
            }
        }
        return;
    }

    for (uint8_t k = 0; k < 4; k++) {
        if (!(visitedLamps & (1 << k))) {
            uint8_t nA = netOfLamp[k][0];
            uint8_t nB = netOfLamp[k][1];

            // Abaikan lampu yang terkorsleting (nA == 255)
            if (nA == 255 || nB == 255) continue;

            if (nA == currentNet) {
                searchCircuitPaths(nB, targetNet, visitedLamps | (1 << k), currentLength + 1, netOfLamp, minChain);
            } else if (nB == currentNet) {
                searchCircuitPaths(nA, targetNet, visitedLamps | (1 << k), currentLength + 1, netOfLamp, minChain);
            }
        }
    }
}

/**
 * Pemindaian Matriks 8 Port Banana / BNC Secara Dinamis & Nodal Analisis
 * Waktu eksekusi: ~250 us. 100% Proteksi Hardware: Hanya drive OUTPUT LOW atau INPUT_PULLUP/PULLDOWN.
 */
void scanBananaMatrix() {
    DSU dsu;
    dsu.init();

    // 1. Uji koneksi langsung ke SRC (+) [3.3V] dan SRC (-) [GND] via Dual-Read (Pull-Up & Pull-Down)
    for (uint8_t i = 0; i < PORT_COUNT; i++) {
        pinMode(BANANA_PINS[i], INPUT_PULLUP);
        delayMicroseconds(15);
        int valPU = digitalRead(BANANA_PINS[i]);

        pinMode(BANANA_PINS[i], INPUT_PULLDOWN);
        delayMicroseconds(15);
        int valPD = digitalRead(BANANA_PINS[i]);

        // Kembalikan ke mode default aman
        pinMode(BANANA_PINS[i], INPUT_PULLUP);

        if (valPU == HIGH && valPD == HIGH) {
            dsu.unite(i, NODE_VCC); // Terhubung ke SRC (+) 3.3V
        } else if (valPU == LOW && valPD == LOW) {
            dsu.unite(i, NODE_GND); // Terhubung ke SRC (-) GND
        }
    }

    // 2. Uji jumper antar-port (Open-Drain LOW Scanning - Dilarang drive HIGH demi proteksi chip)
    for (uint8_t i = 0; i < PORT_COUNT; i++) {
        pinMode(BANANA_PINS[i], OUTPUT);
        digitalWrite(BANANA_PINS[i], LOW);
        delayMicroseconds(15);

        for (uint8_t j = i + 1; j < PORT_COUNT; j++) {
            if (digitalRead(BANANA_PINS[j]) == LOW) {
                dsu.unite(i, j); // Port i dan j terhubung kabel jumper
            }
        }

        // Kembalikan pin i ke INPUT_PULLUP
        pinMode(BANANA_PINS[i], INPUT_PULLUP);
        delayMicroseconds(10);
    }

    // 3. Deteksi Korslet Rel Utama (SRC+ dicolok langsung ke SRC-)
    isSrcShortCircuited = (dsu.find(NODE_VCC) == dsu.find(NODE_GND));
    if (isSrcShortCircuited) {
        for (uint8_t k = 0; k < 4; k++) {
            lpjBwhChain[k] = 0;
            lpjBwhDuty[k]  = 0;
        }
        return;
    }

    // 4. Analisis Rangkaian Graf untuk 4 Beban Lampu Miniatur
    uint8_t netVCC = dsu.find(NODE_VCC);
    uint8_t netGND = dsu.find(NODE_GND);

    uint8_t netOfLamp[4][2];
    uint8_t minChain[4] = {255, 255, 255, 255};

    for (uint8_t k = 0; k < 4; k++) {
        uint8_t p1 = 2 * k;     // Terminal 1: A1, B1, C1, D1
        uint8_t p2 = 2 * k + 1; // Terminal 2: A2, B2, C2, D2
        netOfLamp[k][0] = dsu.find(p1);
        netOfLamp[k][1] = dsu.find(p2);

        // Jika kedua terminal lampu berada di net yang sama, lampu terkorslet sendiri (shorted load)
        if (netOfLamp[k][0] == netOfLamp[k][1]) {
            netOfLamp[k][0] = 255; // Tandai tidak dapat dilalui arus
            netOfLamp[k][1] = 255;
        }
    }

    // Jalankan DFS mencari rantai seri terpendek dari netVCC ke netGND
    searchCircuitPaths(netVCC, netGND, 0, 0, netOfLamp, minChain);

    // 5. Tentukan Kecerahan PWM & Panjang Rantai untuk Masing-Masing Lampu
    for (uint8_t k = 0; k < 4; k++) {
        if (minChain[k] == 1) {
            lpjBwhChain[k] = 1;
            lpjBwhDuty[k]  = 255; // 100% Kecerahan (Paralel Penuh / 1 Lampu Mandiri)
        } else if (minChain[k] == 2) {
            lpjBwhChain[k] = 2;
            lpjBwhDuty[k]  = 128; // 50% Kecerahan (2 Lampu Seri)
        } else if (minChain[k] == 3) {
            lpjBwhChain[k] = 3;
            lpjBwhDuty[k]  = 85;  // 33% Kecerahan (3 Lampu Seri)
        } else if (minChain[k] == 4) {
            lpjBwhChain[k] = 4;
            lpjBwhDuty[k]  = 64;  // 25% Kecerahan (4 Lampu Seri)
        } else {
            lpjBwhChain[k] = 0;
            lpjBwhDuty[k]  = 0;   // 0% (Padam: Rangkaian Putus / Terkorslet)
        }
    }
}

/**
 * 5. LPJ Jalur Bawah (4 LED Simulator Rangkaian):
 *    L1 (D26 / Beban A), L2 (D12 / Beban B), L3 (D33 / Beban C), L4 (D32 / Beban D).
 *    Mendukung kecerahan bertingkat PWM sesuai hukum pembagi tegangan rangkaian seri/paralel.
 */
void updateLowerStreetlights(unsigned long now) {
    static unsigned long lastScanTime = 0;

    // Pemindaian adaptif setiap 40 ms (responsif tanpa membebani CPU)
    if (now - lastScanTime >= 40) {
        lastScanTime = now;
        scanBananaMatrix();
    }

    // Terapkan output PWM ke 4 pin LPJ Bawah
    for (uint8_t k = 0; k < 4; k++) {
        uint8_t targetDuty = (isLowerStreetArmed && !isSrcShortCircuited) ? lpjBwhDuty[k] : 0;
        analogWrite(LPJ_BWH_PINS[k], targetDuty);
    }
}

/**
 * 6. Kendali Motor Driver L298N (2x Dinamo DC 1-Arah, GPIO 15 / Soket J):
 */
void updateMotorDriver() {
    digitalWrite(PIN_MOTOR_DRV, isMotorActive ? HIGH : LOW);
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

        Serial.printf("[CUACA] LDR_DO:%d (%-5s) | SOLAR:%4d (%-5s) || LPJ ATAS: [%d, %d] | LPJ BWH (PWM): [L1:%3d, L2:%3d, L3:%3d, L4:%3d]%s | RUMAH: %-3s | MOTOR: %-3s | TL: %-6s | IP: %s\n",
                      ldrState,
                      isNight ? "MALAM" : "SIANG",
                      rawSolar,
                      isSolarDay ? "SURYA" : "PADAM",
                      digitalRead(PIN_LPJ_ATAS_1) == HIGH ? 1 : 0,
                      digitalRead(PIN_LPJ_ATAS_2) == HIGH ? 1 : 0,
                      lpjBwhDuty[0],
                      lpjBwhDuty[1],
                      lpjBwhDuty[2],
                      lpjBwhDuty[3],
                      isSrcShortCircuited ? " [KORSLET SRC!]" : "",
                      digitalRead(PIN_HOUSE) == HIGH ? "ON " : "OFF",
                      digitalRead(PIN_MOTOR_DRV) == HIGH ? "ON " : "OFF",
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

    // 3. LPJ Jalur Atas (2 Tiang Lampu IR - Siaga Malam Hari)
    doc["street_upper"]["ir1"]  = sensorIR1.isLow(now);
    doc["street_upper"]["ir2"]  = sensorIR2.isLow(now);
    doc["street_upper"]["lpj1"] = (digitalRead(PIN_LPJ_ATAS_1) == HIGH);
    doc["street_upper"]["lpj2"] = (digitalRead(PIN_LPJ_ATAS_2) == HIGH);

    // 4. LPJ Jalur Bawah (4 Lampu Mandiri dengan Kecerahan PWM & Deteksi Korslet)
    doc["street_lower"]["armed"]        = isLowerStreetArmed;
    doc["street_lower"]["short_src"]    = isSrcShortCircuited;
    doc["street_lower"]["lpj_bwh_1"]    = (lpjBwhDuty[0] > 0);
    doc["street_lower"]["lpj_bwh_2"]    = (lpjBwhDuty[1] > 0);
    doc["street_lower"]["lpj_bwh_3"]    = (lpjBwhDuty[2] > 0);
    doc["street_lower"]["lpj_bwh_4"]    = (lpjBwhDuty[3] > 0);
    doc["street_lower"]["duty_1"]       = lpjBwhDuty[0];
    doc["street_lower"]["duty_2"]       = lpjBwhDuty[1];
    doc["street_lower"]["duty_3"]       = lpjBwhDuty[2];
    doc["street_lower"]["duty_4"]       = lpjBwhDuty[3];
    doc["street_lower"]["chain_1"]      = lpjBwhChain[0];
    doc["street_lower"]["chain_2"]      = lpjBwhChain[1];
    doc["street_lower"]["chain_3"]      = lpjBwhChain[2];
    doc["street_lower"]["chain_4"]      = lpjBwhChain[3];

    // 5. Penerangan Pemukiman (Rumah 1 - 4) & Motor Driver L298N
    doc["houses"]["house_all"] = (digitalRead(PIN_HOUSE) == HIGH);
    doc["motor"]["active"]     = (digitalRead(PIN_MOTOR_DRV) == HIGH);

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

        // 2. Publikasi instan jika ada transisi output lampu/sensor/PWM (Fast Response)
        static int lastLpj1 = -1, lastLpj2 = -1;
        static uint8_t lastDuty0 = 255, lastDuty1 = 255, lastDuty2 = 255, lastDuty3 = 255;
        static bool lastShort = false;
        static int lastHouse = -1, lastMotor = -1, lastTl = -1, lastNight = -1;
        int curLpj1  = digitalRead(PIN_LPJ_ATAS_1);
        int curLpj2  = digitalRead(PIN_LPJ_ATAS_2);
        int curHouse = digitalRead(PIN_HOUSE);
        int curMotor = digitalRead(PIN_MOTOR_DRV);
        int curTl    = isSolarDay ? (int)tlState : (yellowBlinkState ? 99 : 98);

        bool stateChanged = (curLpj1 != lastLpj1 || curLpj2 != lastLpj2 ||
                             lpjBwhDuty[0] != lastDuty0 || lpjBwhDuty[1] != lastDuty1 ||
                             lpjBwhDuty[2] != lastDuty2 || lpjBwhDuty[3] != lastDuty3 ||
                             isSrcShortCircuited != lastShort ||
                             curHouse != lastHouse || curMotor != lastMotor || curTl != lastTl || isNight != lastNight);

        if (stateChanged && (now - lastMqttPublishTime >= MQTT_MIN_CHANGE_GAP)) {
            lastLpj1 = curLpj1; lastLpj2 = curLpj2;
            lastDuty0 = lpjBwhDuty[0]; lastDuty1 = lpjBwhDuty[1];
            lastDuty2 = lpjBwhDuty[2]; lastDuty3 = lpjBwhDuty[3];
            lastShort = isSrcShortCircuited;
            lastHouse = curHouse; lastMotor = curMotor;
            lastTl = curTl; lastNight = isNight;
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

    // 2. Status Master Otomasi & LPJ Bawah
    // Saklar toggle fisik memutus daya ke LDR: saat LDR mati, currentLdrState terbaca LOW (Siang)
    isMasterSensorEnabled = isNight;
    isLowerStreetArmed    = isNight;

    // 3. Eksekusi Seluruh Subsistem Diorama
    updateHouseLights();
    updateUpperStreetlights(now);
    updateLowerStreetlights(now);
    updateTrafficLight(now);
    updateMotorDriver();

    // 4. Pelaporan Status ke Serial Monitor
    printTelemetry(now, currentLdrState, currentRawSolar);

    // 5. Penanganan Komunikasi MQTT & Publikasi Telemetri (Non-Blocking)
    handleMQTT(now);

    delay(5); // Loop sampling interval ringan (200 FPS) untuk kestabilan RTOS
}