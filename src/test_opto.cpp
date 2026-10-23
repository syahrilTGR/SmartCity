#include <Arduino.h>

/**
 * ============================================================================
 * PROGRAM PENGUJIAN: Rangkaian Optocoupler PC817 & Traffic Light Bergantian
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
 * - Sinar Surya Aktif (ADC < 2500):
 *   -> Modul Traffic Light berjalan otomatis bergantian:
 *      MERAH (2 detik) -> KUNING (1 detik) -> HIJAU (2 detik) -> loop...
 * - Gelap / Tanpa Sinar (ADC >= 2500):
 *   -> Siklus dihentikan, semua lampu padam (atau siaga standby).
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
constexpr unsigned long PRINT_INTERVAL_MS = 400;

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
    Serial.println("========================================================");
    Serial.println("Pinout:");
    Serial.println(" - Sensor Opto Solar : GPIO 39 (VN - Analog ADC)");
    Serial.println(" - TL Merah          : GPIO 23");
    Serial.println(" - TL Kuning         : GPIO 5");
    Serial.println(" - TL Hijau          : GPIO 13");
    Serial.println("Threshold Solar      : ADC < 2500 (Sinar Terdeteksi)");
    Serial.println("Siklus Lampu         : MERAH (2s) -> KUNING (1s) -> HIJAU (2s)");
    Serial.println("--------------------------------------------------------");
    Serial.println("Arahkan senter HP ke panel surya untuk mengaktifkan TL!\n");
}

void loop() {
    unsigned long now = millis();
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

    // Monitoring Serial periodik
    if (now - lastPrintTime >= PRINT_INTERVAL_MS) {
        lastPrintTime = now;
        const char* stateStr = "OFF";
        if (currentState == TL_RED) stateStr = "MERAH (ON)";
        else if (currentState == TL_YELLOW) stateStr = "KUNING (ON)";
        else if (currentState == TL_GREEN) stateStr = "HIJAU (ON)";

        Serial.printf("[SOLAR] ADC: %4d | Status: %-12s | Traffic Light: %s\n",
                      rawAnalog,
                      isSolarActive ? "TERANG (<2500)" : "GELAP (>=2500)",
                      stateStr);
    }

    delay(20); // Debounce sampling
}
