# Brief Proyek: Smart City ESP32

Dokumen ini memuat konsep arsitektur, daftar komponen, dan skenario otomasi miniatur **Smart City** berbasis mikrokontroler **ESP32**.

---

## 1. Ringkasan Proyek

Miniatur tata kota pintar (*Smart City*) ini dirancang untuk mensimulasikan sistem pencahayaan modern dan edukasi dasar kelistrikan dengan komponen visual utama:
1. **4 Unit Miniatur Rumah**: Masing-masing memiliki 1 unit LED indikator pencahayaan pemukiman.
2. **6 Unit Lampu Penerangan Jalan Umum (LPJ)** yang dibagi menjadi dua zona:
   - **Jalur Atas (2 Lampu Jalan)**: Menyala otomatis saat mendeteksi kendaraan melintas menggunakan 2 sensor IR Proximity mandiri (beroperasi 24/7, siang maupun malam).
   - **Jalur Bawah (4 Lampu Jalan)**: Beroperasi khusus saat malam hari (dipicu sensor cahaya LDR) dan dikendalikan oleh modul simulator edukasi rangkaian listrik berbasis kabel BNC.
3. **1 Unit Dummy Turbin Angin (Pojok Kiri Atas)**:
   - Menggunakan motor DC Tamiya (tipe 130) yang ditenagai langsung dari jalur 5V dengan saklar manual on/off fisik.
4. **1 Unit Lampu Lalu Lintas Tenaga Surya (Traffic Light)**:
   - Menggunakan modul LED VKS 5V (Merah, Kuning, Hijau) yang dikendalikan oleh ESP32.
   - Dilengkapi panel surya mini 5V yang terhubung via **Optocoupler PC817** sebagai pendeteksi ketersediaan energi matahari / siklus siang.

---

## 2. Arsitektur Komponen

| Komponen | Jumlah | Tegangan Kerja | Fungsi Utama |
| :--- | :---: | :---: | :--- |
| **ESP32 Dev Module** | 1 | 3.3V Logic (Input 5V) | Otak kontroler utama seluruh sistem |
| **LED Miniatur Rumah** | 4 | 3.0V (Resistor 47 Ohm) | Pencahayaan interior 4 rumah |
| **LED LPJ Jalur Atas** | 2 | 3.0V (Resistor 47 Ohm) | Lampu jalan atas (dipicu IR Proximity 1 & 2) |
| **LED LPJ Jalur Bawah** | 4 | 3.0V (Resistor 47 Ohm) | Lampu jalan bawah (simulator seri & paralel) |
| **Sensor IR Proximity** | 2 | 3.3V – 5V | Deteksi rintangan/mobil jalur atas 1 & 2 (Active LOW, 24/7) |
| **Sensor Cahaya (LDR)** | 1 | 3.3V | Pengukur kondisi siang/malam (master trigger jalur bawah) |
| **Soket Female BNC & Kabel** | 4+ unit | Logika 3.3V (Sensing) | Media interaksi edukasi rangkaian listrik |
| **Dinamo Tamiya (Motor 130)** | 1 | 5V DC | Dummy turbin angin pojok kiri atas |
| **Saklar Manual On/Off** | 1 | 5V | Pengendali fisik daya dinamo turbin angin |
| **Modul Traffic Light VKS** | 1 | 5V (R, Y, G, GND) | Lampu lalu lintas 3 warna dikontrol ESP32 |
| **Optocoupler PC817** | 1 | 3.3V / 5V | Isolasi pembaca daya panel surya ke GPIO 39 |
| **Panel Surya Mini 5V** | 1 | 5V | Pemanen energi surya simulasi |

---

## 3. Skenario & Logika Operasi Sistem

### A. LPJ Jalur Atas (Otomasi 24/7)
- Dilengkapi **2 Sensor IR Proximity terpisah**:
  - `Sensor IR 1` mengendalikan `LPJ Jalur Atas 1`.
  - `Sensor IR 2` mengendalikan `LPJ Jalur Atas 2`.
- **Logika:** Lampu menyala saat mobil melintas di depan sensor terkait (sinyal LOW terdeteksi), beroperasi 24/7 tanpa memandang siang atau malam hari.

### B. LPJ Jalur Bawah (Simulator Rangkaian Seri & Paralel)
- **Master Enable (Sensor LDR):**
  - **Siang Hari:** Seluruh LPJ jalur bawah dipadamkan secara paksa.
  - **Malam Hari:** Sistem simulator jalur bawah aktif (*armed*).
- **2 Lampu Kiri Bawah (Simulasi Rangkaian Seri):**
  - 4 port female BNC dihubungkan secara seri (*daisy chain*) ke pin sensing `GPIO 4` (`INPUT_PULLUP`).
  - **Semua BNC Terpasang:** Sirkuit loop tertutup $\rightarrow$ kedua lampu kiri bawah menyala.
  - **1 BNC Dicabut:** Sirkuit terbuka $\rightarrow$ kedua lampu kiri bawah langsung padam seketika.
- **2 Lampu Kanan Bawah (Simulasi Rangkaian Paralel):**
  - Menggunakan 2 cabang BNC independen pada `GPIO 16` dan `GPIO 17` (`INPUT_PULLUP`).
  - **Perilaku:** Di malam hari, memasang kabel BNC cabang 1 menyalakan lampu cabang 1; memasang cabang 2 menyalakan lampu cabang 2. Jika salah satu dicabut, hanya lampu terkait yang padam (lampu lainnya tetap menyala).

### C. Turbin Angin Miniatur
- Beroperasi mandiri menggunakan saklar fisik mekanik tanpa membebani pin GPIO ESP32.

### D. Lampu Lalu Lintas Cerdas Tenaga Surya
- **Mode Normal (Siang / Daya Surya Aktif):** ESP32 memutar sekuensial Merah (5 detik) -> Kuning (2 detik) -> Hijau (4 detik) secara terus menerus.
- **Mode Malam (Daya Surya Padam):** ESP32 beralih ke mode siaga lampu kuning berkedip (*flashing yellow*) seperti kota nyata.

---

## 4. Referensi Dokumentasi Modular Terkait

Untuk menghemat ruang konteks dan mempermudah referensi teknis spesifik, detail teknis lanjutan telah dipisahkan ke dalam dokumen modular:

1. **[docs/pinout.md](file:///Volumes/Data%20Shared/Project/SmartCity/docs/pinout.md)**:
   - Tabel pemetaan seluruh pin GPIO ESP32, arah sinyal, dan skema wiring Fritzing.
2. **[docs/power_budget.md](file:///Volumes/Data%20Shared/Project/SmartCity/docs/power_budget.md)**:
   - Analisis kapasitas regulator 3.3V (AMS1117 / xx5r69), kalkulasi resistor LED (100 Ohm), proteksi dinamo Tamiya, dan kalkulasi kapasitas baterai 18650 + step-up 5V.
3. **[docs/traffic_light.md](file:///Volumes/Data%20Shared/Project/SmartCity/docs/traffic_light.md)**:
   - Spesifikasi teknis Traffic Light Opsi A (ESP32 + Optocoupler PC817) dan Opsi B (Hardware NE555 + CD4017).
4. **[docs/keperluan_beli.md](file:///Volumes/Data%20Shared/Project/SmartCity/docs/keperluan_beli.md)**:
   - Rekapitulasi daftar belanja komponen lengkap proyek.
5. **[AGENTS.md](file:///Volumes/Data%20Shared/Project/SmartCity/AGENTS.md)**:
   - Aturan kerja, alur kompilasi PlatformIO, dan konvensi disiplin proyek.
