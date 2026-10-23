/**
 * =============================================================================
 * Program Pengujian Simulator BNC Seri & Paralel + Lampu LPJ Jalur Bawah
 * Proyek: Smart City Miniatur Diorama (ESP32 DevKit v1)
 * =============================================================================
 *
 * Alokasi Pin:
 * 1. SIMULATOR BNC SERI (Sistem Anti-Curang 2 Pasangan):
 *    - Sensing Loop 1   : GPIO 4  (INPUT_PULLUP) -> Lubang BNC 1-2
 *    - Sensing Loop 2   : GPIO 25 (INPUT_PULLUP) -> Lubang BNC 3-4
 *    - Output Lampu Seri: GPIO 26 (OUTPUT)       -> 2 LED Tiang LPJ Kiri Bawah
 *    * Logika: Lampu Seri (GPIO 26) ON jika dan HANYA JIKA kedua kabel BNC
 *      terpasang (GPIO 4 == LOW DAN GPIO 25 == LOW).
 *      Jika salah satu dicabut -> Lampu Seri OFF (kedua tiang mati).
 *
 * 2. SIMULATOR BNC PARALEL (2 Cabang Independen):
 *    - Sensing Cabang 1 : GPIO 16 / RX2 (INPUT_PULLUP) -> Lubang BNC Par 1
 *    - Sensing Cabang 2 : GPIO 17 / TX2 (INPUT_PULLUP) -> Lubang BNC Par 2
 *    - Output Lampu 1   : GPIO 32       (OUTPUT)       -> LPJ Kanan Bawah 1
 *    - Output Lampu 2   : GPIO 33       (OUTPUT)       -> LPJ Kanan Bawah 2
 *    * Logika: Sifat paralel mandiri.
 *      Cabang 1 terpasang (GPIO 16 == LOW) -> Lampu 1 (GPIO 32) ON.
 *      Cabang 2 terpasang (GPIO 17 == LOW) -> Lampu 2 (GPIO 33) ON.
 * =============================================================================
 */

#include <Arduino.h>

// Definisi Pin Sensing BNC Seri
constexpr uint8_t PIN_BNC_SERI_1 = 4;
constexpr uint8_t PIN_BNC_SERI_2 = 25;
constexpr uint8_t PIN_LPJ_SERI   = 26; // 2 LED paralel fisik

// Definisi Pin Sensing BNC Paralel
constexpr uint8_t PIN_BNC_PAR_1  = 16;
constexpr uint8_t PIN_BNC_PAR_2  = 17;
constexpr uint8_t PIN_LPJ_PAR_1  = 32;
constexpr uint8_t PIN_LPJ_PAR_2  = 33;

// Variabel status sebelumnya untuk deteksi trigger
int lastSeri1 = -1, lastSeri2 = -1;
int lastPar1  = -1, lastPar2  = -1;

unsigned long lastPrintTime = 0;
constexpr unsigned long PRINT_INTERVAL_MS = 500;

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("==========================================================");
    Serial.println("   TESTING SIMULATOR BNC SERI & PARALEL + LPJ BAWAH      ");
    Serial.println("==========================================================");
    Serial.println("1. Rangkaian Seri (Anti-Curang):");
    Serial.println("   - Input Loop 1 : GPIO 4  (BNC 1-2)");
    Serial.println("   - Input Loop 2 : GPIO 25 (BNC 3-4)");
    Serial.println("   - Output Seri  : GPIO 26 (2 LED LPJ Seri)");
    Serial.println("   * Syarat ON    : D4 == LOW && D25 == LOW (Kedua kabel terpasang)");
    Serial.println();
    Serial.println("2. Rangkaian Paralel (Mandiri):");
    Serial.println("   - Input Cabang 1: GPIO 16 -> Output LPJ Par 1: GPIO 32");
    Serial.println("   - Input Cabang 2: GPIO 17 -> Output LPJ Par 2: GPIO 33");
    Serial.println("----------------------------------------------------------");
    Serial.println("Mulai pengujian... Silakan colok/cabut kabel simulator BNC!");
    Serial.println();

    // Konfigurasi Input dengan Resistor Pull-Up Internal ESP32
    pinMode(PIN_BNC_SERI_1, INPUT_PULLUP);
    pinMode(PIN_BNC_SERI_2, INPUT_PULLUP);
    pinMode(PIN_BNC_PAR_1,  INPUT_PULLUP);
    pinMode(PIN_BNC_PAR_2,  INPUT_PULLUP);

    // Konfigurasi Output Lampu LPJ
    pinMode(PIN_LPJ_SERI,  OUTPUT);
    pinMode(PIN_LPJ_PAR_1, OUTPUT);
    pinMode(PIN_LPJ_PAR_2, OUTPUT);

    // Pastikan seluruh lampu awal padam
    digitalWrite(PIN_LPJ_SERI,  LOW);
    digitalWrite(PIN_LPJ_PAR_1, LOW);
    digitalWrite(PIN_LPJ_PAR_2, LOW);
}

void loop() {
    // 1. Baca Sinyal BNC Seri (LOW = Kabel terpasang ke GND)
    int seri1 = digitalRead(PIN_BNC_SERI_1);
    int seri2 = digitalRead(PIN_BNC_SERI_2);

    // Logika Anti-Curang Seri: Keduanya WAJIB terpasang
    bool seriComplete = (seri1 == LOW && seri2 == LOW);
    digitalWrite(PIN_LPJ_SERI, seriComplete ? HIGH : LOW);

    // 2. Baca Sinyal BNC Paralel (LOW = Kabel terpasang ke GND)
    int par1 = digitalRead(PIN_BNC_PAR_1);
    int par2 = digitalRead(PIN_BNC_PAR_2);

    // Logika Paralel: Masing-masing cabang mengendalikan lampunya sendiri
    bool par1Active = (par1 == LOW);
    bool par2Active = (par2 == LOW);
    digitalWrite(PIN_LPJ_PAR_1, par1Active ? HIGH : LOW);
    digitalWrite(PIN_LPJ_PAR_2, par2Active ? HIGH : LOW);

    // Cetak log hanya jika terjadi perubahan status fisik colokan BNC
    if (seri1 != lastSeri1 || seri2 != lastSeri2 || par1 != lastPar1 || par2 != lastPar2) {
        Serial.println("-------------------- [EVENT PERUBAHAN] --------------------");
        // Log Seri
        Serial.printf("[SERI] Loop1 (D4): %s | Loop2 (D25): %s => Lampu Seri (D26): %s\n",
                      (seri1 == LOW) ? "TERPASANG" : "TERPUTUS ",
                      (seri2 == LOW) ? "TERPASANG" : "TERPUTUS ",
                      seriComplete   ? "*** ON (MENYALA) ***" : "OFF (PADAM)");

        // Log Paralel
        Serial.printf("[PAR1] Cabang1 (D16): %s => Lampu Par 1 (D32): %s\n",
                      par1Active ? "TERPASANG" : "TERPUTUS ",
                      par1Active ? "ON " : "OFF");
        Serial.printf("[PAR2] Cabang2 (D17): %s => Lampu Par 2 (D33): %s\n",
                      par2Active ? "TERPASANG" : "TERPUTUS ",
                      par2Active ? "ON " : "OFF");
        Serial.println();

        lastSeri1 = seri1;
        lastSeri2 = seri2;
        lastPar1  = par1;
        lastPar2  = par2;
    }

    // Status detak jantung periodik setiap 500ms
    if (millis() - lastPrintTime >= PRINT_INTERVAL_MS) {
        lastPrintTime = millis();
        Serial.printf("[HEARTBEAT] SERI: %s (D4:%d, D25:%d) | PAR1: %s (D16:%d) | PAR2: %s (D17:%d)\n",
                      seriComplete ? "ON " : "OFF", seri1, seri2,
                      par1Active   ? "ON " : "OFF", par1,
                      par2Active   ? "ON " : "OFF", par2);
    }

    delay(20); // Debounce sampling
}
