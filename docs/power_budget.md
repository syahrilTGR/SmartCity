# Analisis Arus & Kelistrikan (Power Budget)

Dokumen ini memuat analisis kelistrikan, batas aman IC regulator 3.3V (ESP32 varian **xx5r69**), kalkulasi resistor LED, proteksi beban dinamo, serta estimasi kapasitas baterai 3.7V dan modul step-up 5V.

---

## 1. Spesifikasi LED & Rumus Resistor

### Karakteristik LED Miniatur
- **Tegangan Jatuh (Vf):** 3.0 Volt (tipe LED 3mm / 5mm)
- **Arus Nominal Maksimum (If):** 20 mA = 0.02 A

### Rumus Dasar (Hukum Ohm)
```text
R = (Vs - Vf) / If
```

| Sumber Tegangan (Vs) | Nilai Resistor (R) | Arus per LED | Total Arus 10 LED | Status & Keamanan |
| :--- | :---: | :---: | :---: | :--- |
| **GPIO ESP32 (3.3V)** | **100 Ohm** | **3.0 mA** | **30 mA** | **Standar Terpilih:** Super aman, dingin, awet, dan terang pas untuk diorama |
| **GPIO ESP32 (3.3V)** | 47 – 68 Ohm | 4.5 – 6.5 mA | 45 – 65 mA | Aman (opsi jika ingin sedikit lebih terang) |
| **GPIO ESP32 (3.3V)** | 15 Ohm | 20.0 mA | 200 mA | **BAHAYA:** Melebihi batas kumulatif GPIO ESP32 (maks 120 mA) |

---

## 2. Analisis Rangkaian Seri vs Pendekatan Sensing ESP32

### Karakteristik Seri Murni (2 LED)
- Jika 2 LED dirangkai secara seri fisik:
  `Vf_total = 3.0V + 3.0V = 6.0 Volt`
- **Kendala Fisik:** Tegangan sumber 3.3V maupun 5V USB **kurang dari 6V**, sehingga 2 LED seri fisik **tidak dapat menyala**.
- **Solusi Desain Proyek:**
  - Proyek ini menggunakan **Pendekatan Logika ESP32 (Sensing)**.
  - Seluruh LED fisik LPJ jalur bawah tetap ditenagai mandiri pada pin GPIO masing-masing secara individual (3.3V) dengan resistor 100 Ohm.
  - Efek rangkaian seri (jika 1 kabel BNC / tombol dicabut maka semua lampu mati) dikendalikan sepenuhnya melalui logika kode program ESP32.
  - **Keuntungan:** Sistem **TIDAK MEMERLUKAN SUMBER DAYA 6V/9V/12V**, cukup 1 catu daya 5V untuk seluruh sistem.

---

## 3. Batas Arus IC Regulator 3.3V ESP32 (Board xx5r69)

ESP32 varian **xx5r69** (NodeMCU ESP32-WROOM-32) umumnya menggunakan regulator LDO:
* **AMS1117-3.3 (SOT-223):** Kapasitas nominal 800 mA – 1000 mA, batas termal kontinu aman ~500 mA.
* **ME6211 / AP2112K (SOT-89/SOT-23-5):** Batas aman ~400 – 500 mA.

### Neraca Arus Sistem Internal:
1. **Konsumsi Chip ESP32:**
   - Dual-Core aktif: ~80 – 120 mA.
   - Transmisi puncak RF/Wi-Fi (*burst*): ~240 – 400 mA.
2. **Batas Kumulatif GPIO ESP32 (Rekomendasi Espressif):**
   - Maksimum per pin: 12 – 20 mA.
   - Total arus seluruh pin GPIO bersamaan: **<= 100 – 120 mA**.
3. **Beban 10 LED Proyek Smart City (Resistor 100 Ohm):**
   - `Total Arus 10 LED = 10 x 3 mA = 30 mA`.
   - Angka 30 mA ini hanya memakai 25% dari batas aman GPIO (120 mA), sehingga regulator 3.3V dan chip ESP32 dipastikan tetap dingin dan bebas dari *brownout reset*.

---

## 4. Proteksi Beban Induktif (Dinamo Tamiya 5V)

Dinamo Tamiya (Motor DC 130) menarik arus kerja 250 – 350 mA dan lonjakan start (*surge*) hingga 1.5 A.

1. **Jalur Daya Terpisah:** Dinamo ditenagai langsung dari jalur **5V**, **TIDAK melalui regulator 3.3V ESP32**.
2. **Dioda Flyback (1N4007):** Dipasang paralel terbalik pada terminal dinamo untuk meredam lonjakan tegangan induktif (*back-EMF*) saat saklar dimatikan.
3. **Kapasitor Decoupling (220µF - 470µF):** Dipasang di jalur daya 5V dinamo untuk mencegah drop tegangan sesaat yang dapat memicu *Brownout Reset* pada ESP32.

---

## 5. Sumber Daya Baterai 3.7V & Modul Step-Up 5V

Sistem dirancang dapat beroperasi secara portabel menggunakan baterai Lithium 3.7V (tipe 18650) yang dinaikkan tegangannya ke 5V melalui modul Step-Up (Boost Converter).

### A. Skema Distribusi Daya
```text
Baterai 3.7V (18650 Paralel) ───> Modul Step-Up 5V (MT3608 / IP5306)
                                        │
                         ┌──────────────┴──────────────┐
                         │                             │
                   Pin VIN ESP32             Saklar Dinamo Tamiya 5V
                         │                             │
               (ESP32, Sensor, 10 LED)        (Motor Kincir Angin)
```

### B. Konsumsi Arus Riil pada Jalur 5V
1. **Sub-Sistem ESP32 & Elektronika:**
   - ESP32 Dual-Core: ~100 mA
   - 10 LED (R 100 Ohm): ~30 mA
   - 2 Sensor IR Proximity: ~35 mA
   - Sensor LDR & Pull-up: ~1 mA
   - *Total Elektronika:* **~166 mA**
2. **Sub-Sistem Dinamo Kincir Angin:**
   - Dinamo Tamiya 130 (Beban Kincir Ringan): **~300 mA**

### C. Total Arus Beban pada 5V:
* **Kondisi 1 — Kincir Angin OFF (Hanya Lampu & Sensor):** `~166 mA` (Daya: ~0.83 Watt)
* **Kondisi 2 — Kincir Angin ON (Sistem Penuh):** `~466 – 500 mA` (Daya: ~2.5 Watt)

### D. Arus yang Ditarik dari Baterai 3.7V
Dengan efisiensi modul step-up sebesar 85%:
```text
Arus Baterai = (5.0V x Arus 5V) / (3.7V x 0.85) ≈ 1.6 x Arus 5V
```
* **Saat Kincir Angin OFF:** Arus baterai = `166 mA x 1.6 ≈ 265 mA` (0.265 A)
* **Saat Kincir Angin ON:** Arus baterai = `500 mA x 1.6 ≈ 800 mA` (0.8 A)

### E. Tabel Estimasi Daya Tahan Baterai (Tipe 18650 @ 2.200 mAh per sel)

| Konfigurasi Baterai 18650 | Total Kapasitas | Daya Tahan (Kincir Angin ON) | Daya Tahan (Kincir Angin OFF) | Rekomendasi Penggunaan |
| :---: | :---: | :---: | :---: | :--- |
| **1 Sel (1P)** | 2.200 mAh | **~2,5 Jam** | **~8 Jam** | Cukup untuk pengujian cepat |
| **2 Sel Paralel (2P)** | **4.400 mAh** | **~5 Jam** | **~16 Jam** | **REKOMENDASI TERBAIK:** Sangat ideal untuk demo / pameran seharian |
| **3 Sel Paralel (3P)** | 6.600 mAh | **~8 Jam** | **~24 Jam** | Ekstra awet untuk operasional non-stop |

### F. Rekomendasi Pilihan Modul Hardware:
1. **Opsi 1: Modul Step-Up MT3608 (2A)**
   - Efisiensi tinggi, trimpot diputar hingga output tepat 5.0V.
   - Memerlukan modul charger terpisah seperti TP4056 dengan proteksi baterai (DW01/FS8205).
2. **Opsi 2: Modul Power Bank DIY (Chip IP5306)**
   - Solusi all-in-one paling praktis: sudah dilengkapi port charging USB Type-C, step-up boost 5V 2A, proteksi kehabisan baterai (*low voltage cut-off*), dan indikator 4 level LED baterai.
