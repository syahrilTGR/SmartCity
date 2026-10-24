/**
 * =============================================================================
 * PROGRAM PENGUJIAN: Saklar Toggle Sensor (Master Enable Otomasi)
 * Proyek: Smart City Miniatur Diorama (ESP32 DevKit v1)
 * =============================================================================
 *
 * Alokasi Pin:
 * - Saklar Toggle Sensor : GPIO 15 (INPUT_PULLUP) -> Soket J_SW_SENS (JST 2-Pin)
 *                          * Pin 1 : GPIO 15
 *                          * Pin 2 : GND
 *
 * Pin Indikator Visual (Opsional):
 * - Lampu Rumah 1 & 2    : GPIO 18 (OUTPUT) -> Ikut menyala saat saklar ON
 *
 * Karakteristik INPUT_PULLUP:
 * - Saklar POSISI ON  (Terhubung ke GND) : digitalRead(15) = LOW (0)  -> Otomasi Sensor AKTIF
 * - Saklar POSISI OFF (Terbuka / Floating): digitalRead(15) = HIGH (1) -> Otomasi Sensor NON-AKTIF
 * =============================================================================
 */

#include <Arduino.h>

// Definisi Pin
constexpr uint8_t PIN_SW_SENS   = 15; // Saklar Toggle SPST (Input Pull-up)
constexpr uint8_t PIN_INDICATOR = 18; // Indikator visual ke Lampu Rumah 1-2

// Variabel status
int lastSwitchState = -1;
unsigned long lastPrintTime = 0;
constexpr unsigned long PRINT_INTERVAL_MS = 400;

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Konfigurasi Pin
    // Menggunakan resistor internal pull-up ESP32 (~45 kOhm)
    pinMode(PIN_SW_SENS, INPUT_PULLUP);
    pinMode(PIN_INDICATOR, OUTPUT);

    digitalWrite(PIN_INDICATOR, LOW);

    Serial.println("\n========================================================");
    Serial.println("   PENGUJIAN SAKLAR TOGGLE SENSOR (MASTER ENABLE)       ");
    Serial.println("========================================================");
    Serial.println("Pinout:");
    Serial.println(" - Saklar Toggle  : GPIO 15 (INPUT_PULLUP)");
    Serial.println(" - Indikator LED  : GPIO 18 (Lampu Rumah 1-2)");
    Serial.println("--------------------------------------------------------");
    Serial.println("Petunjuk:");
    Serial.println("1. Geser saklar ke posisi ON (terhubung ke GND):");
    Serial.println("   -> GPIO 15 terbaca: 0 (LOW)");
    Serial.println("   -> Status: AKTIF (ENABLE), LED Rumah 1-2 MENYALA.");
    Serial.println("2. Geser saklar ke posisi OFF (terbuka):");
    Serial.println("   -> GPIO 15 terbaca: 1 (HIGH)");
    Serial.println("   -> Status: MATI (DISABLE), LED Rumah 1-2 PADAM.\n");
}

void loop() {
    unsigned long now = millis();

    // Baca status saklar (LOW = ON / Tertutup ke GND)
    int switchState = digitalRead(PIN_SW_SENS);
    bool isEnabled = (switchState == LOW);

    // Kontrol indikator visual
    digitalWrite(PIN_INDICATOR, isEnabled ? HIGH : LOW);

    // Event Trigger jika saklar digeser
    if (switchState != lastSwitchState) {
        lastSwitchState = switchState;
        Serial.println("\n---------------- [PERUBAHAN POSISI SAKLAR] ----------------");
        if (isEnabled) {
            Serial.println(">>> SAKLAR: [ ON ] (GPIO 15: LOW)  -> OTOMASI SENSOR: AKTIF (ENABLE)   <<<");
            Serial.println(">>> LED Indikator (GPIO 18) MENYALA.                                     <<<");
        } else {
            Serial.println(">>> SAKLAR: [ OFF ] (GPIO 15: HIGH) -> OTOMASI SENSOR: NON-AKTIF (DISABLE) <<<");
            Serial.println(">>> LED Indikator (GPIO 18) PADAM.                                       <<<");
        }
        Serial.println("-----------------------------------------------------------\n");
    }

    // Monitoring Serial periodik
    if (now - lastPrintTime >= PRINT_INTERVAL_MS) {
        lastPrintTime = now;
        Serial.printf("[SWITCH D15] Nilai: %d (%-4s) | Status: %-15s | Indikator D18: %s\n",
                      switchState,
                      (switchState == LOW) ? "LOW" : "HIGH",
                      isEnabled ? "AKTIF (ON)" : "NON-AKTIF (OFF)",
                      isEnabled ? "ON " : "OFF");
    }

    delay(20); // Debounce sampling saklar mekanik
}
