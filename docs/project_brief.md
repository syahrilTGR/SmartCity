# Brief Proyek: Smart City ESP32

Dokumen ini memuat konsep arsitektur, daftar komponen, dan skenario otomasi miniatur **Smart City** berbasis mikrokontroler **ESP32**.

---

## 1. Ringkasan Proyek

Miniatur tata kota pintar (*Smart City*) ini dirancang untuk mensimulasikan sistem pencahayaan modern dan edukasi dasar kelistrikan dengan komponen visual utama:
1. **4 Unit Miniatur Rumah**: Memiliki 4 LED pencahayaan pemukiman yang dikelompokkan menjadi 2 zona (Rumah 1 & 2 di GPIO 18, Rumah 3 & 4 di GPIO 19).
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
| **LED Miniatur Rumah** | 4 | 3.0V (Resistor 100 Ohm) | Pencahayaan interior 4 rumah |
| **LED LPJ Jalur Atas** | 2 | 3.0V (Resistor 100 Ohm) | Lampu jalan atas (dipicu IR Proximity 1 & 2) |
| **LED LPJ Jalur Bawah** | 4 | 3.0V (Resistor 100 Ohm) | Lampu jalan bawah (simulator seri & paralel) |
| **Sensor IR Proximity** | 2 | 3.3V – 5V | Deteksi rintangan/mobil jalur atas 1 & 2 (Active LOW, 24/7) |
| **Sensor Cahaya (LDR)** | 1 | 3.3V | Pengukur kondisi siang/malam (master trigger jalur bawah) |
| **Soket Female BNC & Kabel** | 4 pasang | Logika 3.3V (Sensing) | Media interaksi edukasi rangkaian listrik (2 pasang Seri, 2 pasang Paralel) |
| **Dinamo Tamiya (Motor 130)** | 1 | 5V DC | Dummy turbin angin pojok kiri atas |
| **Saklar Manual Kincir** | 1 | 5V | Pengendali fisik daya dinamo turbin angin |
| **Saklar Power Utama** | 1 | 7.4V / Baterai | Master switch pemutus daya baterai 2S sebelum Step-Down |
| **Saklar Toggle Sensor** | 1 | 3.3V Logic | Saklar manual ke GPIO 15 untuk mengaktifkan/bypass logika sensor |
| **Modul Traffic Light VKS** | 1 | 5V (R, Y, G, GND) | Lampu lalu lintas 3 warna dikontrol ESP32 |
| **Optocoupler PC817** | 1 | 3.3V / 5V | Isolasi pembaca daya panel surya ke GPIO 39 |
| **Panel Surya Mini 5V** | 1 | 5V | Pemanen energi surya simulasi |
| **Baterai 18650 2S + BMS 2S** | 1 paket | 7.4V (8.4V Full) | Sumber daya utama portabel dengan proteksi |
| **Modul Step-Down 5V (MP1584EN)**| 1 | Input 7.4V, Output 5V | Penurun tegangan efisiensi tinggi (Buck Converter) |

---

## 3. Skenario & Logika Operasi Sistem

### A. LPJ Jalur Atas (Otomasi 24/7)
- Dilengkapi **2 Sensor IR Proximity terpisah**:
  - `Sensor IR 1` mengendalikan `LPJ Jalur Atas 1`.
  - `Sensor IR 2` mengendalikan `LPJ Jalur Atas 2`.
- **Logika:** Lampu menyala saat mobil melintas di depan sensor terkait (sinyal LOW terdeteksi), beroperasi 24/7 tanpa memandang siang atau malam hari.

### B. LPJ Jalur Bawah (Simulator Rangkaian Seri & Paralel)
- **Master Enable (Sensor LDR & Saklar Toggle):**
  - **Saklar Toggle Sensor (GPIO 15):** Jika OFF, otomasi sensor dinonaktifkan (mode manual). Jika ON, sistem otomasi sensor aktif.
  - **Sensor LDR (GPIO 36):**
    - **Siang Hari:** Seluruh LPJ jalur bawah dipadamkan secara paksa.
    - **Malam Hari:** Sistem simulator jalur bawah aktif (*armed*).
- **2 Lampu Kiri Bawah (Simulasi Rangkaian Seri - Anti Curang):**
  - Kedua LED seri dikontrol bersamaan dari **1 pin (GPIO 26)** dan diparalel secara fisik (masing-masing R 100 Ohm).
  - Menggunakan 2 pasang sensing digital independen untuk mencegah kecurangan bypass kabel langsung:
    - Pasangan 1 (BNC 1-2): Terhubung ke `GPIO 4` (`INPUT_PULLUP`).
    - Pasangan 2 (BNC 3-4): Terhubung ke `GPIO 25` (`INPUT_PULLUP`).
  - **Logika:** Lampu seri hanya menyala jika **keduanya terpasang** (`GPIO 4 == LOW` DAN `GPIO 25 == LOW`). Jika salah satu dicabut atau dicolok pintas 1 kabel dari ujung ke ujung, lampu seri tetap padam.
- **2 Lampu Kanan Bawah (Simulasi Rangkaian Paralel):**
  - Menggunakan 2 cabang BNC independen pada `GPIO 16` dan `GPIO 17` (`INPUT_PULLUP`).
  - Output LED: Lampu Paralel 1 pada `GPIO 33`, Lampu Paralel 2 pada `GPIO 32`.
  - **Perilaku:** Di malam hari, memasang kabel BNC cabang 1 menyalakan lampu cabang 1; memasang cabang 2 menyalakan lampu cabang 2. Jika salah satu dicabut, hanya lampu terkait yang padam (lampu lainnya tetap menyala).

### C. Turbin Angin Miniatur
- Beroperasi mandiri menggunakan saklar fisik mekanik tanpa membebani pin GPIO ESP32, ditenagai langsung dari jalur 5V output Step-Down dengan proteksi Dioda Flyback 1N4007 dan Kapasitor 470 uF.

### D. Lampu Lalu Lintas Cerdas Tenaga Surya
- **Mode Normal (Siang / Daya Surya Aktif via Optocoupler GPIO 39):** ESP32 memutar sekuensial Merah (GPIO 23: 5 detik) -> Kuning (GPIO 5: 2 detik) -> Hijau (GPIO 13: 4 detik) secara terus menerus.
- **Mode Malam (Daya Surya Padam):** ESP32 beralih ke mode siaga lampu kuning berkedip (*flashing yellow*) seperti kota nyata.

---

## 4. Referensi Dokumentasi Modular Terkait

Untuk menghemat ruang konteks dan mempermudah referensi teknis spesifik, detail teknis lanjutan telah dipisahkan ke dalam dokumen modular:

1. **[docs/pinout.md](file:///Volumes/Data%20Shared/Project/SmartCity/docs/pinout.md)**:
   - Tabel pemetaan seluruh pin GPIO ESP32, arah sinyal, dan skema wiring Fritzing.
2. **[docs/power_budget.md](file:///Volumes/Data%20Shared/Project/SmartCity/docs/power_budget.md)**:
   - Analisis kapasitas regulator 3.3V (AMS1117 / xx5r69), kalkulasi resistor LED (100 Ohm), proteksi dinamo Tamiya, dan kalkulasi kapasitas baterai 18650 2S (7.4V) + Step-Down 5V.
3. **[docs/traffic_light.md](file:///Volumes/Data%20Shared/Project/SmartCity/docs/traffic_light.md)**:
   - Spesifikasi teknis Traffic Light Opsi A (ESP32 + Optocoupler PC817) dan Opsi B (Hardware NE555 + CD4017).
4. **[docs/pcb_wiring.md](file:///Volumes/Data%20Shared/Project/SmartCity/docs/pcb_wiring.md)**:
   - Panduan perakitan PCB Matrix, tata letak komponen on-board, dan pemetaan konektor Molex JST-XH (2-Pin, 3-Pin, 4-Pin).
5. **[docs/keperluan_beli.md](file:///Volumes/Data%20Shared/Project/SmartCity/docs/keperluan_beli.md)**:
   - Rekapitulasi daftar belanja komponen lengkap proyek.
6. **[AGENTS.md](file:///Volumes/Data%20Shared/Project/SmartCity/AGENTS.md)**:
   - Aturan kerja, alur kompilasi PlatformIO, dan konvensi disiplin proyek.

