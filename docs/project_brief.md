# Brief Proyek: Smart City ESP32

Dokumen ini memuat konsep arsitektur, daftar komponen, dan skenario otomasi miniatur **Smart City** berbasis mikrokontroler **ESP32**.

> Sumber kebenaran yang berlaku: `src/main.cpp` (revisi 10-port BNC dan driver motor L298N, commit `e36c33c`), diagram `docs/pcb_matrix_layout_10port.svg`, dan `docs/matriks_logika_8port_bnc.md`. Dokumen ini disinkronkan ke ketiganya pada 9 Nov 2026. Kalau ada yang bertentangan, kode yang menang.

---

## 1. Ringkasan Proyek

Miniatur tata kota pintar (*Smart City*) ini dirancang untuk mensimulasikan sistem pencahayaan modern dan edukasi dasar kelistrikan dengan komponen visual utama:
1. **4 Unit Miniatur Rumah**: 4 LED pencahayaan pemukiman digabung ke satu pin, GPIO 2 (Soket [L]). Menyala otomatis saat malam hari, masing-masing LED memakai resistor 100 Ohm, arus total sekitar 12 mA.
2. **6 Unit Lampu Penerangan Jalan Umum (LPJ)** yang dibagi menjadi dua zona:
   - **Jalur Atas (2 Lampu Jalan)**: Menyala otomatis saat mendeteksi kendaraan melintas menggunakan 2 sensor IR Proximity mandiri. Hanya bersiaga saat malam hari (LDR), siang hari dipadamkan untuk efisiensi energi.
   - **Jalur Bawah (4 Lampu Jalan)**: Simulator rangkaian listrik adaptif 10-port. Tiap lampu dikendalikan mandiri (GPIO 26, 12, 33, 32) dengan kecerahan PWM yang mengikuti hasil analisis rangkaian yang dirangkai siswa lewat kabel BNC. Aktif hanya saat malam hari.
3. **1 Unit Turbin Angin (Pojok Kiri Atas)**:
   - Menggunakan motor DC Tamiya (tipe 130) yang kini digerakkan lewat driver **L298N**, dikendalikan ESP32 dari GPIO 15 (Soket [J], IN1+IN3). Default menyala, bisa dimatikan lewat MQTT atau Serial.
4. **1 Unit Lampu Lalu Lintas Tenaga Surya (Traffic Light)**:
   - Menggunakan modul LED VKS 5V (Merah, Kuning, Hijau) yang dikendalikan oleh ESP32.
   - Dilengkapi panel surya mini 5V yang terhubung via **Optocoupler PC817** sebagai pendeteksi ketersediaan energi matahari / siklus siang.

---

## 2. Arsitektur Komponen

| Komponen | Jumlah | Tegangan Kerja | Fungsi Utama |
| :--- | :---: | :---: | :--- |
| **ESP32 Dev Module** | 1 | 3.3V Logic (Input 5V) | Otak kontroler utama seluruh sistem |
| **LED Miniatur Rumah** | 4 | 3.0V (Resistor 100 Ohm) | Pencahayaan interior 4 rumah, digabung ke GPIO 2 |
| **LED LPJ Jalur Atas** | 2 | 3.0V (Resistor 100 Ohm) | Lampu jalan atas (dipicu IR Proximity 1 & 2, malam hari saja) |
| **LED LPJ Jalur Bawah** | 4 | 3.0V (Resistor 100 Ohm) | Lampu jalan bawah (simulator adaptif, kecerahan PWM) |
| **Sensor IR Proximity** | 2 | 3.3V – 5V | Deteksi rintangan/mobil jalur atas 1 & 2 (Active LOW, siaga malam hari) |
| **Sensor Cahaya (LDR)** | 1 | 3.3V | Modul LM393 digital DO ke GPIO 36. DO HIGH berarti malam, DO LOW berarti siang. VCC modul diputus saklar toggle seri |
| **Port BNC Simulator** | 10 | Logika 3.3V | 2 terminal sumber (SRC+ 3.3V via R 1k, SRC- GND) + 8 terminal beban A1, A2, B1, B2, C1, C2, D1, D2 ke GPIO 4, 25, 16, 17, 21, 22, 18, 19 |
| **Resistor Seri BNC** | 8 | - | Resistor 150 Ohm di tiap jalur port A1 s/d D2, tepat di belakang soket BNC |
| **Driver Motor L298N** | 1 | Logika 5V | Driver 2 kanal dinamo DC 1 arah, dikendalikan GPIO 15 (IN1+IN3, Soket [J]) |
| **Dinamo Tamiya (Motor 130)** | 1 | 5V DC | Turbin angin pojok kiri atas, digerakkan via L298N |
| **Saklar Toggle Otomasi** | 1 | 3.3V | Saklar seri di jalur VCC modul LDR (Soket [I]). OFF berarti LDR mati fisik, sistem membaca siang dan semua otomasi lampu padam |
| **Saklar Power Utama** | 1 | 7.4V / Baterai | Master switch pemutus daya baterai 2S sebelum Step-Down |
| **Modul Traffic Light VKS** | 1 | 5V (R, Y, G, GND) | Lampu lalu lintas 3 warna dikontrol ESP32 |
| **Optocoupler PC817** | 1 | 3.3V / 5V | Isolasi pembaca daya panel surya ke GPIO 39 |
| **Panel Surya Mini 5V** | 1 | 5V | Pemanen energi surya simulasi |
| **Baterai 18650 2S + BMS 2S** | 1 paket | 7.4V (8.4V Full) | Sumber daya utama portabel dengan proteksi |
| **Modul Step-Down 5V (MP1584EN)**| 1 | Input 7.4V, Output 5V | Penurun tegangan efisiensi tinggi (Buck Converter) |

---

## 3. Skenario & Logika Operasi Sistem

### A. LPJ Jalur Atas (Otomasi Malam Hari)
- Dilengkapi **2 Sensor IR Proximity terpisah**:
  - `Sensor IR 1` mengendalikan `LPJ Jalur Atas 1` (GPIO 14).
  - `Sensor IR 2` mengendalikan `LPJ Jalur Atas 2` (GPIO 27).
- **Logika:** Lampu hanya bersiaga saat malam hari (`isNight == true`). Saat mobil melintas di depan sensor terkait (sinyal LOW terdeteksi), lampu menyala dan ditahan 1 detik setelah mobil lewat (off-delay 1000 ms). Siang hari kedua lampu dipadamkan.

### B. LPJ Jalur Bawah (Simulator Rangkaian Adaptif 10-Port)
- **Master Enable:** Simulator hanya aktif saat malam hari (`isLowerStreetArmed == isNight`). Siang hari seluruh LPJ jalur bawah padam.
- **Cara kerja:** ESP32 memindai 8 port beban tiap 40 ms (baca ganda pull-up/pull-down, lalu analisis graf Union-Find + DFS). Tiap lampu menyala sesuai rangkaian yang dirangkai siswa:
  - **1 lampu mandiri / paralel penuh:** tegangan penuh 3.3V, PWM 100% (duty 255).
  - **2 lampu seri:** tegangan terbagi, PWM 50% (duty 128).
  - **3 lampu seri:** PWM 33% (duty 85).
  - **4 lampu seri:** PWM 25% (duty 64).
  - **Rangkaian putus atau lampu terkorslet (kedua kakinya di net yang sama):** lampu padam, PWM 0%.
- **Deteksi korslet sumber:** Kalau `SRC+` dan `SRC-` terhubung langsung (satu net yang sama menurut hasil pemindaian graf), keempat lampu bawah dipadamkan dan sistem memberi peringatan di dashboard web / Serial Monitor. Deteksi ini murni dari analisis graf, bukan dari pin sense terpisah.
- **Pemetaan beban ke lampu:** Lampu 1 (GPIO 26) dari pasangan A1/A2, Lampu 2 (GPIO 12) dari B1/B2, Lampu 3 (GPIO 33) dari C1/C2, Lampu 4 (GPIO 32) dari D1/D2.

### C. Penerangan Rumah
- Keempat LED rumah digabung ke **GPIO 2** (Soket [L]).
- **Logika:** Menyala saat malam hari (`isNight == true`), padam saat siang. Tidak ada sensor atau saklar terpisah untuk rumah.

### D. Turbin Angin via Driver L298N
- Dua kanal dinamo DC 1 arah dikendalikan bersama dari **GPIO 15** (Soket [J], IN1+IN3 L298N, bisa ON/OFF dan PWM).
- Default motor aktif berputar (`isMotorActive = true`). Status bisa diubah lewat MQTT atau Serial Monitor. Motor tidak digate kondisi siang/malam, jadi turbin berputar terus selama diaktifkan.
- Catu daya motor (+5V switched) masuk lewat Soket [G].

### E. Lampu Lalu Lintas Cerdas Tenaga Surya
- **Mode Normal (Siang / Daya Surya Aktif via Optocoupler GPIO 39):** ESP32 memutar sekuensial Merah (GPIO 23: 5 detik) -> Kuning (GPIO 5: 2 detik) -> Hijau (GPIO 13: 4 detik) secara terus menerus.
- **Mode Malam (Daya Surya Padam):** ESP32 beralih ke mode siaga lampu kuning berkedip (*flashing yellow*) seperti kota nyata.

### F. Saklar Toggle Otomasi (Master Sensor)
- Saklar toggle dipasang seri di jalur VCC 3.3V ke modul LDR (Soket [I]), bukan lagi ke pin GPIO.
- Saat saklar OFF, modul LDR mati fisik sehingga sistem membaca siang (`isNight = false`) dan seluruh otomasi lampu (rumah, LPJ atas, LPJ bawah) padam seketika. Traffic light dan motor tidak terpengaruh saklar ini.

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
7. **`docs/matriks_logika_8port_bnc.md`**:
   - Spesifikasi teknis simulator 10-port BNC: pemetaan pin bebas strapping, algoritma baca ganda, analisis nodal, proteksi listrik, dan tabel skenario pengujian.
8. **`docs/pcb_matrix_layout_10port.svg`**:
   - Diagram denah soket JST PCB Matrix revisi 10-port yang berlaku. Ini denah visual yang dipakai untuk perakitan, bukan `pcb_matrix_layout.svg` yang lama.
