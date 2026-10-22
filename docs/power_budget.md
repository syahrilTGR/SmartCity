# Analisis Arus & Kelistrikan (Power Budget)

Dokumen ini memuat analisis kelistrikan, batas aman IC regulator 3.3V (ESP32 varian **xx5r69**), kalkulasi resistor LED, proteksi beban dinamo, serta perancangan catu daya **Baterai 18650 Konfigurasi 2S (7.4V) + Modul Step-Down 5V**.

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
  - Kedua LED LPJ seri kiri bawah **dikontrol bersamaan dari pin GPIO 26** dan **diparalel secara fisik** (masing-masing memiliki resistor 100 Ohm ke GND).
  - Arus total 2 LED pada GPIO 26 hanya `3 mA + 3 mA = 6 mA` (sangat aman di bawah batas pin 12 – 20 mA).
  - Efek rangkaian seri (jika 1 kabel BNC dicabut maka semua lampu mati) dikendalikan sepenuhnya melalui logika mikrokontroler ESP32 dengan sensor ganda anti-curang pada **GPIO 4** dan **GPIO 25**.
  - **Keuntungan:** Sistem **TIDAK MEMERLUKAN SUMBER DAYA 6V/9V/12V**, cukup 1 catu daya 5V dari output Step-Down.

---

## 3. Batas Arus IC Regulator 3.3V ESP32 (Board xx5r69)

ESP32 varian **xx5r69** (NodeMCU ESP32-WROOM-32) umumnya menggunakan regulator LDO:
* **AMS1117-3.3 (SOT-223):** Kapasitas nominal 800 mA – 1000 mA, batas termal kontinu aman ~500 mA.
* **ME6211 / AP2112K (SOT-89/SOT-23-5):** Batas aman ~400 – 500 mA.

### Neraca Arus Sistem Internal (3.3V):
1. **Konsumsi Chip ESP32:**
   - Dual-Core aktif: ~80 – 120 mA.
   - Transmisi puncak RF/Wi-Fi (*burst*): ~240 – 400 mA.
2. **Batas Kumulatif GPIO ESP32 (Rekomendasi Espressif):**
   - Maksimum per pin: 12 – 20 mA (maksimum absolut 40 mA).
   - Beban 2 LED paralel pada GPIO 18, 19, dan 26 hanya menarik ~6 – 12 mA per pin (sangat aman).
   - Total arus seluruh pin GPIO bersamaan: **<= 100 – 120 mA**.
3. **Beban 10 LED Proyek Smart City (Resistor 100 Ohm):**
   - `Total Arus 10 LED = 10 x 3 mA = 30 mA`.
   - Hanya memakai 25% dari kapasitas aman GPIO (120 mA).


---

## 4. Proteksi Beban Induktif (Dinamo Tamiya 5V)

Dinamo Tamiya (Motor DC 130) menarik arus kerja 250 – 350 mA dan lonjakan start (*surge*) hingga 1.5 A.

1. **Jalur Daya Terpisah:** Dinamo ditenagai langsung dari jalur **5V (Output Step-Down)**, **TIDAK melalui regulator 3.3V ESP32**.
2. **Dioda Flyback (1N4007):** Dipasang paralel terbalik pada terminal dinamo untuk meredam lonjakan tegangan induktif (*back-EMF*) saat saklar dimatikan.
3. **Kapasitor Decoupling (220µF - 470µF):** Dipasang di jalur daya 5V dinamo untuk mencegah drop tegangan sesaat yang dapat memicu *Brownout Reset* pada ESP32.

---

## 5. Sumber Daya Baterai 2S (7.4V) & Modul Step-Down 5V

Sistem beralih menggunakan konfigurasi **Baterai 18650 Seri (2S = 7.4V nominal / 8.4V full)** yang diturunkan tegangannya ke 5.0V melalui modul **Step-Down (Buck Converter)**.

### A. Mengapa Step-Down 2S Jauh Lebih Unggul daripada Step-Up?
1. **Efisiensi Sangat Tinggi:** Modul Step-Down (Buck) memiliki efisiensi **90% – 94%**, bekerja jauh lebih dingin dibanding Step-Up.
2. **Arus Baterai Jauh Lebih Rendah:** Karena tegangan baterai (7.4V) lebih tinggi dari tegangan output (5.0V), maka **arus yang ditarik dari baterai justru lebih kecil dibanding arus beban 5V**, sehingga sel baterai tidak terbebani dan tegangan tidak drop saat dinamo menyala.

### B. Skema Distribusi Daya 2S
```text
Baterai 18650 (2S = 7.4V) ───[ Modul BMS 2S ]───[ Saklar Power Utama ]───> [ Modul Step-Down 5V (MP1584EN) ]
                                                                                         │
                                                                           Jalur Daya +5V Bersama (Stabil)
                                                                                         │
                                                                           ┌─────────────┴─────────────┐
                                                                           │                           │
                                                                     Pin VIN ESP32           Saklar Dinamo Kincir 5V
```

### C. Konsumsi Arus Riil pada Jalur 5V (Beban Output)
1. **Sub-Sistem ESP32 & Elektronika:**
   - ESP32 Dual-Core: ~100 mA
   - 10 LED Miniatur (R 100 Ohm): ~30 mA
   - 2 Sensor IR Proximity: ~35 mA
   - Modul Traffic Light VKS: ~15 mA (1 warna aktif)
   - Sensor LDR & Pull-up: ~1 mA
   - *Total Elektronika:* **~181 mA**
2. **Sub-Sistem Dinamo Kincir Angin:**
   - Dinamo Tamiya 130 (Beban Kincir Ringan): **~300 mA**

### D. Total Arus Beban pada 5V:
* **Kondisi 1 — Kincir Angin OFF (Hanya Lampu & Sensor):** `~181 mA` (Daya: ~0.90 Watt)
* **Kondisi 2 — Kincir Angin ON (Sistem Penuh):** `~481 – 500 mA` (Daya: ~2.5 Watt)

### E. Arus yang Ditarik dari Baterai 2S (7.4V)
Dengan efisiensi modul step-down sebesar 90%:
```text
Daya Baterai = Daya Beban 5V / 0.90
Arus Baterai (I_bat) = (5.0V x I_beban) / (7.4V x 0.90) ≈ 0.75 x I_beban
```
* **Saat Kincir Angin OFF:** Arus baterai = `181 mA x 0.75 ≈ 135 mA` (0.135 A)
* **Saat Kincir Angin ON:** Arus baterai = `500 mA x 0.75 ≈ 375 mA` (0.375 A)

### F. Tabel Estimasi Daya Tahan Baterai 2S (Baterai 18650 @ 2.200 mAh)

| Konfigurasi Baterai | Kapasitas Paket | Daya Tahan (Kincir Angin ON) | Daya Tahan (Kincir Angin OFF) | Keterangan |
| :---: | :---: | :---: | :---: | :--- |
| **2S1P (2 Sel Seri)** | **2.200 mAh (16.28 Wh)** | **~5,5 sampai 6 Jam** | **~16 sampai 17 Jam** | **STANDAR TERPILIH:** Sangat cukup untuk pameran / demo |
| **2S2P (4 Sel Seri-Paralel)** | 4.400 mAh (32.56 Wh) | **~11 sampai 12 Jam** | **~32 Jam** | Daya tahan ekstra lama |

### G. Rekomendasi Modul Hardware Power 2S:
1. **Modul BMS 2S (8.4V 3A – 5A):**
   - Wajib dipasang melintasi baterai 2S untuk proteksi *overcharge*, *overdischarge* (< 3V per sel), dan *short-circuit*.
2. **Modul Step-Down Mini MP1584EN (3A):**
   - Bentuk sangat mungil, efisiensi tinggi (> 90%), trimpot diputar hingga output tepat **5.0V**.
3. **Saklar Power Utama (Master Switch):**
   - Dipasang pada kabel positif (+) baterai 2S sebelum masuk ke input modul step-down agar tidak ada arus siaga yang terbuang saat sistem dimatikan.
