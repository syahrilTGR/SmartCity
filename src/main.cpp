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

// =============================================================================
// DEFINISI PINOUT (Sesuai docs/pinout.md & docs/pcb_wiring.md)
// =============================================================================

// Sensor Input-Only (ESP32 ADC1 / Digital Input)
constexpr uint8_t PIN_IR1         = 34; // IR Proximity 1 (Jalur Atas 1)
constexpr uint8_t PIN_IR2         = 35; // IR Proximity 2 (Jalur Atas 2)
constexpr uint8_t PIN_LDR         = 36; // Sensor Cahaya LDR (VP, ADC1_CH0)
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

// Ambang Batas Histeresis Sensor LDR (Mencegah kedipan saat batas senja)
constexpr int LDR_ENTER_NIGHT       = 3650; // Masuk malam jika ADC < 3650 (cukup bayangan tangan / redup ringan)
constexpr int LDR_EXIT_NIGHT        = 3850; // Kembali siang jika ADC > 3850 (terkena cahaya lampu ruangan langsung)

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

// =============================================================================
// HELPER FUNCTIONS
// =============================================================================

void setTrafficLight(bool r, bool y, bool g) {
    digitalWrite(PIN_TL_RED,    r ? HIGH : LOW);
    digitalWrite(PIN_TL_YELLOW, y ? HIGH : LOW);
    digitalWrite(PIN_TL_GREEN,  g ? HIGH : LOW);
}

// =============================================================================
// SETUP
// =============================================================================

void setup() {
    Serial.begin(115200);
    delay(1000);

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
    // Inisialisasi awal kondisi malam/siang berdasarkan pembacaan sensor saat boot
    int bootLdr = analogRead(PIN_LDR);
    isNight = (bootLdr < LDR_ENTER_NIGHT);

    setTrafficLight(false, false, false);
    tlStateStartTime = millis();

    Serial.println("Inisialisasi selesai. Seluruh filter debounce & histeresis aktif!");
    Serial.println("----------------------------------------------------------------");
}

// =============================================================================
// LOGIKA SUBSISTEM DENGAN FILTERING LENGKAP
// =============================================================================

/**
 * 1. Filter Histeresis untuk LDR (Siang / Malam):
 *    Menghilangkan efek lampu rumah berkedip-kedip saat pencahayaan di ambang batas.
 */
void updateLdrHysteresis(int rawLdr) {
    if (!isNight && rawLdr < LDR_ENTER_NIGHT) {
        isNight = true; // Beralih ke Malam
    } else if (isNight && rawLdr > LDR_EXIT_NIGHT) {
        isNight = false; // Beralih ke Siang
    }
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
void printTelemetry(unsigned long now, int rawLdr, int rawSolar) {
    if (now - lastTelemetryTime >= TELEMETRY_INTERVAL) {
        lastTelemetryTime = now;

        const char* tlStr = "SIAGA (Kuning Kedip)";
        if (isSolarDay) {
            if (tlState == TL_RED)         tlStr = "MERAH ";
            else if (tlState == TL_YELLOW) tlStr = "KUNING";
            else if (tlState == TL_GREEN)  tlStr = "HIJAU ";
        }

        Serial.printf("[CUACA] LDR:%4d (%-5s) | SOLAR:%4d (%-5s) | SW_SENS: %-3s || LPJ ATAS: [%d, %d] | LPJ BWH: [%s | P1:%d, P2:%d] | RUMAH: %-3s | TL: %s\n",
                      rawLdr,
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
                      tlStr);
    }
}

// =============================================================================
// MAIN LOOP (Non-Blocking Real-Time Architecture)
// =============================================================================

void loop() {
    unsigned long now = millis();

    // 1. Sampling & Filter Histeresis Sensor Analog
    int rawLdr   = analogRead(PIN_LDR);
    int rawSolar = analogRead(PIN_OPTO_SOLAR);

    updateLdrHysteresis(rawLdr);
    updateSolarHysteresis(rawSolar);

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
    printTelemetry(now, rawLdr, rawSolar);

    delay(5); // Loop sampling interval ringan (200 FPS) untuk kestabilan RTOS
}