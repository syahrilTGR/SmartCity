/**
 * =============================================================================
 * PROGRAM PENGUJIAN: Modul Sensor Cahaya LDR 4-Pin (IC LM393 + Trimpot Biru)
 * (MODE DIGITAL READ - KEBAL NOISE WI-FI)
 * Proyek: Smart City Miniatur Diorama (ESP32 DevKit v1)
 * =============================================================================
 *
 * Koneksi Kabel Modul LDR 4-Pin ke ESP32:
 * - VCC : Hubungkan ke 3.3V ESP32
 * - GND : Hubungkan ke GND ESP32
 * - DO  : Hubungkan ke GPIO 36 (VP, Input-Only)
 * - AO  : Dibiarkan KOSONG / NC (Tidak Dipakai)
 *
 * Output Lampu Rumah:
 * - Rumah 1 & 2 : GPIO 18
 * - Rumah 3 & 4 : GPIO 19
 * =============================================================================
 */

#include <Arduino.h>

// Definisi Pin
constexpr uint8_t PIN_LDR         = 36; // Pin DO modul LDR masuk ke GPIO 36 (VP)
constexpr uint8_t PIN_HOUSE_1_2   = 18; // Output Lampu Rumah 1 & 2
constexpr uint8_t PIN_HOUSE_3_4   = 19; // Output Lampu Rumah 3 & 4

// Logging periodik
unsigned long lastPrintTime = 0;
constexpr unsigned long PRINT_INTERVAL_MS = 250;

// Status terakhir kondisi
int lastDigitalState = -1;

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
    Serial.println("   PENGUJIAN MODUL SENSOR LDR 4-PIN (LM393 + TRIMPOT)   ");
    Serial.println("           (MODE DIGITAL READ - KEBAL WI-FI)            ");
    Serial.println("========================================================");
    Serial.println("[PANDUAN WIRING]");
    Serial.println("  * VCC modul -> 3.3V ESP32");
    Serial.println("  * GND modul -> GND ESP32");
    Serial.println("  * DO modul  -> GPIO 36 (VP)");
    Serial.println("  * AO modul  -> KOSONGKAN (NC)\n");
    Serial.println("[CARA KALIBRASI TRIMPOT BIRU]");
    Serial.println("  1. Posisikan sensor di bawah cahaya ruangan biasa (Siang).");
    Serial.println("  2. Putar trimpot biru perlahan dengan obeng sampai LED");
    Serial.println("     indikator DO di modul berubah padam/nyala tepat di ambang batas.");
    Serial.println("  3. Tutup sensor LDR dengan tangan (Malam), amati reaksi lampu.");
    Serial.println("--------------------------------------------------------\n");
}

void loop() {
    unsigned long now = millis();

    // Baca logika digital murni dari pin DO modul
    int digitalVal = digitalRead(PIN_LDR);

    // Pada sebagian besar modul komparator LM393:
    // - Saat TERANG (Siang): DO = LOW (0)
    // - Saat GELAP (Malam) : DO = HIGH (1)
    bool isNight = (digitalVal == HIGH);

    // Kontrol Lampu Rumah
    setHouseLights(isNight);

    // Notifikasi instan jika terjadi pergantian status digital
    if (digitalVal != lastDigitalState) {
        lastDigitalState = digitalVal;
        Serial.println("\n----------------- [EVENT PERUBAHAN CAHAYA] -----------------");
        if (isNight) {
            Serial.println(">>> [DO = HIGH (1)] -> KONDISI: MALAM HARI -> LAMPU RUMAH MENYALA (ON) <<<");
        } else {
            Serial.println(">>> [DO = LOW  (0)] -> KONDISI: SIANG HARI -> LAMPU RUMAH PADAM (OFF) <<<");
        }
        Serial.println("------------------------------------------------------------\n");
    }

    // Tampilkan data periodik ke Serial Monitor
    if (now - lastPrintTime >= PRINT_INTERVAL_MS) {
        lastPrintTime = now;

        Serial.printf("[MODUL LDR DIGITAL] Pin DO: %d (%-4s) | Kondisi: %-5s | Lampu Rumah: %-3s\n",
                      digitalVal,
                      (digitalVal == HIGH) ? "HIGH" : "LOW",
                      isNight ? "MALAM" : "SIANG",
                      isNight ? "ON " : "OFF");
    }

    delay(20);
}
