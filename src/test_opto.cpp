#include <Arduino.h>

/**
 * ============================================================================
 * PROGRAM PENGUJIAN: Rangkaian Optocoupler PC817 & Panel Surya (Smart City)
 * ============================================================================
 * 
 * Pin Yang Diuji:
 * - GPIO 39 (VN) : Input sinyal dari Optocoupler PC817 (Collector)
 *                  Memerlukan Resistor Eksternal Pull-up 10k Ohm ke 3.3V.
 * 
 * Pin Indikator Tambahan (Traffic Light - jika sudah terpasang):
 * - GPIO 23 : LED Merah   (Menyala saat Gelap / Tidak ada sinar)
 * - GPIO 5  : LED Kuning  (Berkedip siaga)
 * - GPIO 13 : LED Hijau   (Menyala saat Sinar Terdeteksi)
 * 
 * Logika Sinyal PC817:
 * - Ada Sinar Matahari : LED PC817 Aktif -> Transistor ON -> GPIO 39 = LOW (0)
 * - Gelap / Ditutup   : LED PC817 Mati  -> Transistor OFF -> GPIO 39 = HIGH (1)
 * ============================================================================
 */

// Definisi Pin
const int PIN_OPTO_SOLAR = 39;  // VN pin (Input-Only)
const int PIN_TL_RED     = 23;  // LED Merah Traffic Light
const int PIN_TL_YELLOW  = 5;   // LED Kuning Traffic Light
const int PIN_TL_GREEN   = 13;  // LED Hijau Traffic Light

// Variabel status
int lastState = -1;
unsigned long lastPrintTime = 0;
const unsigned long PRINT_INTERVAL = 500; // Cetak serial monitor tiap 500ms

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Konfigurasi Pin
  // CATATAN: GPIO 39 adalah pin INPUT murni (tidak mendukung internal pull-up),
  // pastikan resistor eksternal 10k Ohm terhubung dari GPIO 39 ke 3.3V.
  pinMode(PIN_OPTO_SOLAR, INPUT);

  // Inisialisasi LED Indikator Traffic Light (jika sudah dihubungkan)
  pinMode(PIN_TL_RED, OUTPUT);
  pinMode(PIN_TL_YELLOW, OUTPUT);
  pinMode(PIN_TL_GREEN, OUTPUT);

  // Matikan semua LED di awal
  digitalWrite(PIN_TL_RED, LOW);
  digitalWrite(PIN_TL_YELLOW, LOW);
  digitalWrite(PIN_TL_GREEN, LOW);

  Serial.println("\n==================================================");
  Serial.println("   PENGUJIAN SENSOR OPTOCOUPLER PC817 & SOLAR    ");
  Serial.println("==================================================");
  Serial.println("Petunjuk Pengujian:");
  Serial.println("1. Arahkan senter HP atau lampu ke Panel Surya:");
  Serial.println("   -> GPIO 39 harus terbaca: LOW (0)");
  Serial.println("   -> LED Hijau (GPIO 13) menyala.");
  Serial.println("2. Tutup rapat Panel Surya dengan tangan/kertas:");
  Serial.println("   -> GPIO 39 harus terbaca: HIGH (1)");
  Serial.println("   -> LED Merah (GPIO 23) menyala.");
  Serial.println("--------------------------------------------------");
}

void loop() {
  // Baca status digital dan analog dari GPIO 39
  int digitalVal = digitalRead(PIN_OPTO_SOLAR);
  int rawAnalog  = analogRead(PIN_OPTO_SOLAR); // Pembacaan ADC (0 - 4095)
  unsigned long now = millis();

  // Deteksi Perubahan Kondisi Sinyal (Event Triggered)
  if (digitalVal != lastState) {
    lastState = digitalVal;
    Serial.println("\n>>> [PERUBAHAN TERDETEKSI] <<<");
    if (digitalVal == LOW) {
      Serial.println("[STATUS]: TERANG / SINAR SURYA AKTIF! (Sinyal: LOW)");
      digitalWrite(PIN_TL_GREEN, HIGH);
      digitalWrite(PIN_TL_RED, LOW);
      digitalWrite(PIN_TL_YELLOW, LOW);
    } else {
      Serial.println("[STATUS]: GELAP / TANPA SINAR SURYA! (Sinyal: HIGH)");
      digitalWrite(PIN_TL_GREEN, LOW);
      digitalWrite(PIN_TL_RED, HIGH);
      digitalWrite(PIN_TL_YELLOW, LOW);
    }
  }

  // Cetak status periodik setiap 500 ms
  if (now - lastPrintTime >= PRINT_INTERVAL) {
    lastPrintTime = now;
    Serial.print("Waktu: ");
    Serial.print(now / 1000);
    Serial.print("s | GPIO 39 Digital: ");
    Serial.print(digitalVal == LOW ? "0 (LOW) " : "1 (HIGH)");
    Serial.print(" | ADC Raw: ");
    Serial.print(rawAnalog);
    Serial.print(" | Kondisi: ");
    if (digitalVal == LOW) {
      Serial.println("TERANG (Panel Menghasilkan Arus)");
    } else {
      Serial.println("GELAP (Tidak Ada Arus Solar)");
    }
  }
}
