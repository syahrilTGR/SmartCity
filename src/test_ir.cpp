/**
 * =============================================================================
 * Program Pengujian Sensor IR Proximity 1 & 2 dan Lampu LPJ Jalur Atas 1 & 2
 * Proyek: Smart City Miniatur Diorama (ESP32 DevKit v1)
 * =============================================================================
 *
 * Alokasi Pin:
 * - IR Sensor 1       : GPIO 34 (Input-Only)
 * - IR Sensor 2       : GPIO 35 (Input-Only)
 * - LPJ Jalur Atas 1  : GPIO 14 (Output, LED via Resistor 100 Ohm)
 * - LPJ Jalur Atas 2  : GPIO 27 (Output, LED via Resistor 100 Ohm)
 *
 * Logika Pengujian:
 * - Modul IR obstacle umumnya Active LOW:
 *   * Terdeteksi objek / mobil -> OUT = LOW (0)  -> Lampu ON (HIGH)
 *   * Tidak ada objek          -> OUT = HIGH (1) -> Lampu OFF (LOW)
 * - Masing-masing sensor mengontrol lampunya sendiri secara independen:
 *   * IR 1 (GPIO 34) mengendalikan LPJ Atas 1 (GPIO 14)
 *   * IR 2 (GPIO 35) mengendalikan LPJ Atas 2 (GPIO 27)
 * =============================================================================
 */

#include <Arduino.h>

// Definisi Pin Input Sensor IR
constexpr uint8_t PIN_IR1 = 34;
constexpr uint8_t PIN_IR2 = 35;

// Definisi Pin Output Lampu LPJ Atas
constexpr uint8_t PIN_LPJ_ATAS_1 = 14;
constexpr uint8_t PIN_LPJ_ATAS_2 = 27;

// Status terakhir untuk mendeteksi perubahan
int lastStateIR1 = -1;
int lastStateIR2 = -1;

unsigned long lastPrintTime = 0;
constexpr unsigned long PRINT_INTERVAL_MS = 500;

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("==================================================");
    Serial.println("   TESTING SENSOR IR 1 & IR 2 + LPJ ATAS 1 & 2   ");
    Serial.println("==================================================");
    Serial.println("Pinout:");
    Serial.println(" - IR 1           : GPIO 34 (Input)");
    Serial.println(" - IR 2           : GPIO 35 (Input)");
    Serial.println(" - LPJ Atas 1     : GPIO 14 (Output)");
    Serial.println(" - LPJ Atas 2     : GPIO 27 (Output)");
    Serial.println("--------------------------------------------------");
    Serial.println("Mulai pengujian... Letakkan tangan/objek di depan IR!");
    Serial.println();

    // Konfigurasi pin input (GPIO 34 & 35 adalah input-only pada ESP32)
    pinMode(PIN_IR1, INPUT);
    pinMode(PIN_IR2, INPUT);

    // Konfigurasi pin output LED
    pinMode(PIN_LPJ_ATAS_1, OUTPUT);
    pinMode(PIN_LPJ_ATAS_2, OUTPUT);

    // Inisialisasi awal lampu MATI
    digitalWrite(PIN_LPJ_ATAS_1, LOW);
    digitalWrite(PIN_LPJ_ATAS_2, LOW);
}

void loop() {
    // Baca status digital sensor (modul IR umumnya LOW saat terdeteksi halangan)
    int stateIR1 = digitalRead(PIN_IR1);
    int stateIR2 = digitalRead(PIN_IR2);

    // Logika IR 1 -> LPJ Atas 1
    bool ir1Detected = (stateIR1 == LOW);
    digitalWrite(PIN_LPJ_ATAS_1, ir1Detected ? HIGH : LOW);

    // Logika IR 2 -> LPJ Atas 2
    bool ir2Detected = (stateIR2 == LOW);
    digitalWrite(PIN_LPJ_ATAS_2, ir2Detected ? HIGH : LOW);

    // Cetak log jika terjadi perubahan status
    if (stateIR1 != lastStateIR1 || stateIR2 != lastStateIR2) {
        Serial.print("[TRIGGER] ");
        Serial.print("IR 1: ");
        Serial.print(ir1Detected ? "DETEKSI OBJEK (LOW)" : "BEBAS (HIGH)");
        Serial.print(" -> LPJ 1: ");
        Serial.print(ir1Detected ? "MENYALA (ON)" : "MATI (OFF)");
        Serial.print(" | ");
        Serial.print("IR 2: ");
        Serial.print(ir2Detected ? "DETEKSI OBJEK (LOW)" : "BEBAS (HIGH)");
        Serial.print(" -> LPJ 2: ");
        Serial.println(ir2Detected ? "MENYALA (ON)" : "MATI (OFF)");

        lastStateIR1 = stateIR1;
        lastStateIR2 = stateIR2;
    }

    // Status detak jantung periodik setiap 500ms
    if (millis() - lastPrintTime >= PRINT_INTERVAL_MS) {
        lastPrintTime = millis();
        Serial.printf("[STATUS] IR1 (D34): %d | LPJ1 (D14): %s  ||  IR2 (D35): %d | LPJ2 (D27): %s\n",
                      stateIR1,
                      ir1Detected ? "ON " : "OFF",
                      stateIR2,
                      ir2Detected ? "ON " : "OFF");
    }

    delay(20); // Debounce sampling singkat
}
