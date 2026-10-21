# Technical Specification: Solar-Powered Traffic Light (VKS Module)

Dokumen ini memuat spesifikasi teknis, skema rangkaian, dan opsi implementasi sistem lampu lalu lintas miniatur (Merah, Kuning, Hijau) bertenaga surya untuk modul LED **VKS Traffic Lights**.

Tersedia dua opsi perancangan:
* **OPSI A (Terpilih / Implementasi Utama):** Terintegrasi dengan **ESP32** menggunakan sensor daya surya via **Optocoupler PC817** (Cerdas, hemat komponen, dan durasi lampu realistis).
* **OPSI B (Opsi Alternatif Hardware Standalone):** Rangkaian diskrit mandiri tanpa mikrokontroler menggunakan **Timer NE555 + Decade Counter CD4017**.

---

## 1. OPSI A (Terpilih): Kontrol ESP32 via Optocoupler PC817

### A. Konsep Kerja
1. **Deteksi Daya Surya:** Panel surya mini 5V menyuplai arus ke sisi masukan LED inframerah pada **Optocoupler PC817** (melalui resistor pembatas 330 Ohm).
2. **Isolasi Aman ke ESP32:** Sisi fototransistor PC817 terhubung ke pin **GPIO 39 (VN)** ESP32 dengan resistor pull-up 10k Ohm ke 3.3V.
   - Saat ada sinar matahari (panel menghasilkan daya): Optocoupler ON -> `GPIO 39` terbaca **LOW** (Mode Siang / Normal).
   - Saat tidak ada sinar matahari / malam: Optocoupler OFF -> `GPIO 39` terbaca **HIGH** (Mode Malam / Siaga).
3. **Kendali LED VKS:** Tiga pin output ESP32 (`GPIO 23`, `GPIO 5`, `GPIO 13`) mengendalikan langsung pin Merah, Kuning, dan Hijau pada modul VKS 5V.

### B. Daftar Komponen Opsi A
| No | Komponen | Tipe / Spesifikasi | Jumlah | Fungsi |
| :-: | :--- | :--- | :-: | :--- |
| 1 | **Modul Traffic Light VKS** | Modul 5V (R, Y, G, GND) | 1 unit | Tampilan 3 lampu lalu lintas miniatur |
| 2 | **Panel Surya Mini** | 5V (100mA – 500mA) | 1 unit | Sensor pemanen daya matahari |
| 3 | **Optocoupler** | PC817 (DIP-4) | 1 buah | Pengaman isolasi sinyal panel surya ke ESP32 |
| 4 | **Resistor R_opto** | 330 – 470 Ohm (1/4W) | 1 buah | Pembatas arus LED internal optocoupler |
| 5 | **Resistor Pull-Up** | 10 kOhm (1/4W) | 1 buah | Pull-up sinyal kolektor ke 3.3V ESP32 |

### C. Alokasi Pin ESP32 untuk Traffic Light
* **`GPIO 39 (VN)`** : Input Digital (Membaca sinyal panel surya via Optocoupler PC817)
* **`GPIO 23`** : Output Digital -> Pin **R (Merah)** Modul VKS
* **`GPIO 5`** : Output Digital -> Pin **Y (Kuning)** Modul VKS
* **`GPIO 13`** : Output Digital -> Pin **G (Hijau)** Modul VKS
* **GND Modul VKS** : Terhubung ke **GND ESP32**

### D. Skema Pengkabelan Optocoupler PC817
```text
[ SISI PANEL SURYA ]                            [ SISI ESP32 ]
Panel Surya (+) ───[ Resistor 330 Ohm ]─── (Pin 1 Anoda)
                                               PC817     (Pin 4 Kolektor) ─── GPIO 39 (VN)
                                              OPTO-IC             │
                                                         [ Resistor 10k ke 3.3V ]
Panel Surya (-) ────────────────────────── (Pin 2 Katoda)
                                                         (Pin 3 Emitor)   ─── GND ESP32
```

### E. Keunggulan Opsi A:
- **Durasi Waktu Realistis:** Waktu tiap lampu dapat diatur di kodingan program (contoh: Merah 5 detik, Kuning 2 detik, Hijau 4 detik).
- **Mode Malam Cerdas:** Saat panel surya mendeteksi malam hari, ESP32 dapat mengaktifkan mode lampu kuning berkedip (*flashing yellow*) seperti lalu lintas kota nyata pada malam hari.
- **Sangat Hemat Biaya:** Menghilangkan kebutuhan IC NE555, IC CD4017, potensiometer, dan kapasitor timing.

---

## 2. OPSI B (Alternatif Standalone): Rangkaian Timer NE555 + Counter CD4017

Opsi ini bekerja murni menggunakan perangkat keras elektronika analog/digital tanpa melibatkan mikrokontroler.

### A. Daftar Komponen Opsi B
| Komponen | Jumlah | Nilai / Tipe | Fungsi |
| :--- | :---: | :---: | :--- |
| **Solar Panel Mini** | 1 | 5V (200mA – 500mA) | Sumber daya mandiri |
| **IC Pulse Generator** | 1 | NE555 Timer (DIP-8) | Pembangkit clock kontinu (Astable Mode) |
| **IC Decade Counter** | 1 | CD4017 (DIP-16) | Pencacah sekuensial pembagi langkah LED |
| **Modul Traffic Light** | 1 | VKS Module 5V (R, Y, G, GND) | Lampu lalu lintas miniatur 3 warna |
| **Resistor R1** | 1 | 10 kOhm (1/4 Watt) | Pembatas arus pengisian kapasitor NE555 |
| **Potensiometer** | 1 | 100 kOhm | Pengatur kecepatan transisi lampu lalu lintas |
| **Kapasitor Timing C1** | 1 | 10 µF (Elektrolit, min 10V) | Penentu frekuensi clock timer NE555 |
| **Kapasitor Filter C2** | 1 | 10 nF (Keramik 103) | Peredam noise pin 5 NE555 (opsional) |
| **Kapasitor Penyangga** | 1 | 470 µF – 1000 µF (Elektrolit) | Penstabil tegangan output panel surya |

### B. Skema Perkabelan Opsi B

1. **Rangkaian NE555 (Astable Clock Generator):**
   * **Pin 1 (GND):** Ke Negatif Panel Surya.
   * **Pin 2 (Trigger) & Pin 6 (Threshold):** Dijumper bersama, lalu ke Potensiometer 100k dan Positif (+) Kapasitor 10 µF.
   * **Pin 3 (Output Clock):** Dihubungkan ke **Pin 14 (Clock In) CD4017**.
   * **Pin 4 (Reset) & Pin 8 (VCC):** Ke jalur **+5V** Panel Surya.
   * **Pin 5 (Control Voltage):** Ke GND via kapasitor 10 nF.
   * **Pin 7 (Discharge):** Ke +5V via Resistor 10k, dan ke Pin 2/6 via Potensiometer 100k.
   * **Negatif (-) Kapasitor 10 µF:** Ke GND.

2. **Rangkaian CD4017 (3-Step Loop):**
   * **Pin 16 (VDD):** Ke +5V.
   * **Pin 8 (VSS) & Pin 13 (Clock Inhibit):** Ke GND.
   * **Pin 14 (Clock In):** Menerima pulsa dari Pin 3 NE555.
   * **Koneksi Reset (Kritis):** **Pin 7 (Output 3 / Q3)** dihubungkan langsung ke **Pin 15 (Reset)** agar counter seketika kembali ke Merah setelah Hijau selesai (mencegah jeda padam 7 siklus).

3. **Koneksi ke Modul VKS:**
   * **Pin 3 (Q0) CD4017:** Ke Pin **R (Red)** VKS
   * **Pin 2 (Q1) CD4017:** Ke Pin **Y (Yellow)** VKS
   * **Pin 4 (Q2) CD4017:** Ke Pin **G (Green)** VKS
   * **GND VKS:** Ke GND bersama

### C. Diagram Skema Pengkabelan Opsi B (Quick Reference)
```text
PANEL SURYA 5V (+) ───┬───────────────────────┬────────────┬───────────── (Pin 16 VDD CD4017)
                      │                       │            │
                  [ R 10k ]              (Pin 4 & 8     [ C Penyangga ]
                      │                   VCC NE555)       470µF
                 (Pin 7 Disch)                             │
                      │                                   GND
                 [ Pot 100k ]
                      │
   ┌──────────────────┴──────────────────┐
   │                                     │
(Pin 2 & 6 Trig/Thresh)              (Pin 3 Output) ──────> (Pin 14 Clock In CD4017)
   │
 [ C 10µF ]
   │
  GND

                                      CD4017:
                                      Pin 3 (Q0)  ───────> Pin R (Red) VKS
                                      Pin 2 (Q1)  ───────> Pin Y (Yellow) VKS
                                      Pin 4 (Q2)  ───────> Pin G (Green) VKS
                                      Pin 7 (Q3)  ───┬───> (Reset Loop)
                                                     │
                                      Pin 15 (RST) ──┘
                                      Pin 8 & 13  ───────> GND
                                      VKS GND     ───────> GND
```
