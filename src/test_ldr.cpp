/**
 * =============================================================================
 * PROGRAM PENGUJIAN: Sensor Cahaya LDR (Siang / Malam) & Lampu Rumah
 * Proyek: Smart City Miniatur Diorama (ESP32 DevKit v1)
 * =============================================================================
 *
 * Alokasi Pin:
 * - Sensor LDR (VP)   : GPIO 36 (Input-Only, ADC1 Channel 0)
 *                       Rangkaian pembagi tegangan: 3.3V - LDR - GPIO 36 - R 10k - GND
 * - Lampu Rumah 1 & 2 : GPIO 18 (Output, LED Rumah via Resistor 47-51 Ohm)
 * - Lampu Rumah 3 & 4 : GPIO 19 (Output, LED Rumah via Resistor 47-51 Ohm)
 *
 * Karakteristik Rangkaian Pembagi Tegangan:
 * - Kondisi Terang (Siang) : Hambatan LDR turun -> Tegangan GPIO 36 naik -> ADC TINGGI (~2500 - 4095)
 * - Kondisi Gelap (Malam)  : Hambatan LDR naik  -> Tegangan GPIO 36 turun -> ADC RENDAH (~0 - 1500)
 *
 * Logika Pengujian:
 * - Jika Gelap / Malam (ADC < THRESHOLD):
 *   -> Lampu Rumah 1-2 (GPIO 18) & Rumah 3-4 (GPIO 19) MENYALA (HIGH).
 * - Jika Terang / Siang (ADC >= THRESHOLD):
 *   -> Lampu Rumah PADAM (LOW).
 * =============================================================================
 */

#include <Arduino.h>

// Definisi Pin
constexpr uint8_t PIN_LDR         = 36; // VP pin (Input-Only ADC1_CH0)
constexpr uint8_t PIN_HOUSE_1_2   = 18; // Output Lampu Rumah 1 & 2
constexpr uint8_t PIN_HOUSE_3_4   = 19; // Output Lampu Rumah 3 & 4

// Ambang Batas Default Siang/Malam (Dapat disesuaikan berdasarkan pencahayaan ruangan)
constexpr int LDR_THRESHOLD = 2000;

// Logging periodik
unsigned long lastPrintTime = 0;
constexpr unsigned long PRINT_INTERVAL_MS = 300;

// Status terakhir kondisi
bool lastIsNight = false;

void setHouseLights(bool state) {
    digitalWrite(PIN_HOUSE_1_2, state ? HIGH : LOW);
    digitalWrite(PIN_HOUSE_3_4, state ? HIGH : LOW);
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Konfigurasi Pin
    pinMode(PIN_LDR, INPUT);
    pinMode(PIN_HOUSE_1_2, OUTPUT);
    pinMode(PIN_HOUSE_3_4, OUTPUT);

    // Kondisi awal lampu padam
    setHouseLights(false);

    Serial.println("\n========================================================");
    Serial.println("   PENGUJIAN SENSOR LDR (SIANG / MALAM) & LAMPU RUMAH   ");
    Serial.println("========================================================");
    Serial.println("Pinout:");
    Serial.println(" - Sensor LDR      : GPIO 36 (VP - Analog ADC1)");
    Serial.println(" - Lampu Rumah 1-2 : GPIO 18 (Output)");
    Serial.println(" - Lampu Rumah 3-4 : GPIO 19 (Output)");
    Serial.printf ("Threshold Malam   : ADC < %d\n", LDR_THRESHOLD);
    Serial.println("--------------------------------------------------------");
    Serial.println("Petunjuk:");
    Serial.println("1. Tutup sensor LDR dengan jari/kertas (Simulasi Malam):");
    Serial.println("   -> Nilai ADC turun, Lampu Rumah 1-4 akan MENYALA.");
    Serial.println("2. Lepas penutup atau sinari LDR (Simulasi Siang):");
    Serial.println("   -> Nilai ADC naik, Lampu Rumah 1-4 akan PADAM.\n");
}

void loop() {
    unsigned long now = millis();

    // Baca nilai ADC mentah (rentang 12-bit: 0 s/d 4095)
    int rawADC = analogRead(PIN_LDR);

    // Hitung perkiraan tegangan (0 - 3.3 Volt)
    float voltage = (rawADC / 4095.0f) * 3.3f;

    // Tentukan status malam vs siang
    bool isNight = (rawADC < LDR_THRESHOLD);

    // Kontrol Lampu Rumah
    setHouseLights(isNight);

    // Notifikasi jika terjadi pergantian status
    if (isNight != lastIsNight) {
        lastIsNight = isNight;
        Serial.println("\n----------------- [EVENT PERUBAHAN CAHAYA] -----------------");
        if (isNight) {
            Serial.printf(">>> KONDISI: MALAM HARI (ADC: %d < %d) -> LAMPU RUMAH MENYALA (ON) <<<\n", rawADC, LDR_THRESHOLD);
        } else {
            Serial.printf(">>> KONDISI: SIANG HARI (ADC: %d >= %d) -> LAMPU RUMAH PADAM (OFF) <<<\n", rawADC, LDR_THRESHOLD);
        }
        Serial.println("------------------------------------------------------------\n");
    }

    // Tampilkan data periodik ke Serial Monitor
    if (now - lastPrintTime >= PRINT_INTERVAL_MS) {
        lastPrintTime = now;

        // Visual progress bar nilai ADC sederhana
        int barLength = map(rawADC, 0, 4095, 0, 20);
        char bar[25];
        for (int i = 0; i < 20; i++) {
            bar[i] = (i < barLength) ? '#' : '.';
        }
        bar[20] = '\0';

        Serial.printf("[LDR] ADC: %4d | Volt: %4.2fV | Bar: [%s] | Suasana: %-6s | Lampu Rumah: %s\n",
                      rawADC,
                      voltage,
                      bar,
                      isNight ? "MALAM" : "SIANG",
                      isNight ? "ON (MENYALA)" : "OFF");
    }

    delay(20);
}
