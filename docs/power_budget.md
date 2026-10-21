# Analisis Arus & Kelistrikan (Power Budget)

Dokumen ini memuat analisis kelistrikan, batas aman IC regulator 3.3V (ESP32 varian **xx5r69**), kalkulasi resistor LED, serta pencegahan beban lebih (*overcurrent/brownout*).

---

## 1. Spesifikasi LED & Rumus Resistor

### Karakteristik LED
- **Tegangan Jatuh ($V_f$):** $3.0\text{ V}$
- **Arus Kerja ($I_f$):** $20\text{ mA} = 0.02\text{ A}$ (Maksimum nominal LED 3mm/5mm)

### Rumus Dasar (Hukum Ohm)
$$R = \frac{V_s - V_f}{I_f}$$

| Sumber Daya ($V_s$) | Hitungan Teoritis ($20\text{ mA}$) | Nilai Resistor Rekomendasi Praktis | Arus Kerja Nyata | Catatan |
| :--- | :---: | :---: | :---: | :--- |
| **GPIO ESP32 (3.3V)** | $R = \frac{3.3 - 3.0}{0.02} = 15\ \Omega$ | **$47\ \Omega - 68\ \Omega$** | $\approx 4.5 - 6.5\text{ mA}$ | **Paling aman untuk beban 10 LED bersamaan** |
| **Rail USB / 5V** | $R = \frac{5.0 - 3.0}{0.02} = 100\ \Omega$ | **$100\ \Omega - 150\ \Omega$** | $\approx 13 - 20\text{ mA}$ | Digunakan jika menyalakan LED via transistor/driver |

---

## 2. Analisis Rangkaian Seri vs Pendekatan Sensing ESP32

### Karakteristik Seri Murni (2 LED)
- Jika 2 LED dirangkai secara seri fisik:
  $$V_{f,\text{total}} = 3.0\text{ V} + 3.0\text{ V} = 6.0\text{ V}$$
- **Kendala Fisik:** Tegangan sumber 3.3V maupun 5V USB **kurang dari 6V**, sehingga 2 LED seri fisik **tidak dapat menyala**.
- **Solusi Desain Proyek:**
  - Proyek ini menggunakan **Pendekatan Logika ESP32 (Sensing)**.
  - Seluruh LED fisik LPJ jalur bawah tetap ditenagai mandiri pada pin GPIO masing-masing secara individual ($3.3\text{V}$).
  - Efek rangkaian seri (jika 1 kabel BNC dicabut maka semua lampu mati) dikendalikan sepenuhnya melalui logika kode program ESP32.
  - **Keuntungan:** Sistem **TIDAK MEMERLUKAN SUMBER DAYA 6V/9V/12V**, cukup 1 catu daya 5V untuk seluruh sistem.

---

## 3. Batas Arus IC Regulator 3.3V ESP32 (Board xx5r69)

ESP32 varian **xx5r69** (NodeMCU ESP32-WROOM-32) umumnya menggunakan regulator LDO:
* **AMS1117-3.3 (SOT-223):** Kapasitas nominal $800\text{ mA} - 1000\text{ mA}$, batas termal kontinu aman $\approx 500\text{ mA}$.
* **ME6211 / AP2112K (SOT-89/SOT-23-5):** Batas aman $\approx 400\text{–}500\text{ mA}$.

### Neraca Arus Sistem:
1. **Konsumsi Chip ESP32:**
   - Dual-Core aktif: $\approx 80\text{–}120\text{ mA}$.
   - Transmisi puncak RF/Wi-Fi (*burst*): $\approx 240\text{–}400\text{ mA}$.
2. **Batas Kumulatif GPIO ESP32 (Rekomendasi Espressif):**
   - Maksimum per pin: $12\text{–}20\text{ mA}$.
   - Total arus seluruh pin GPIO: **$\le 100\text{–}120\text{ mA}$**.
3. **Beban 10 LED Proyek Smart City:**
   - Jika memakai $20\text{ mA}$ per LED $\rightarrow$ $10 \times 20\text{ mA} = 200\text{ mA}$ *(Berbahaya bagi GPIO dan membuat regulator panas).*
   - **Dengan Resistor $47\ \Omega - 68\ \Omega$ per LED:**
     $$\text{Total Arus 10 LED} = 10 \times \approx 6\text{ mA} = \mathbf{60\text{ mA}}$$
     *(Sangat aman, jauh di bawah batas GPIO 120mA dan menyisakan banyak kuota daya bagi regulator 3.3V).*

---

## 4. Proteksi Beban Induktif (Dinamo Tamiya 5V)

Dinamo Tamiya (Motor DC 130) menarik arus kerja $150\text{–}350\text{ mA}$ dan lonjakan start (*surge*) hingga $1.5\text{ A}$.

1. **Jalur Daya Terpisah:** Dinamo ditenagai langsung dari jalur **5V USB**, **TIDAK melalui regulator 3.3V ESP32**.
2. **Dioda Flyback (1N4007):** Dipasang paralel terbalik pada terminal dinamo untuk meredam lonjakan tegangan induktif (*back-EMF*) saat saklar dimatikan.
3. **Kapasitor Decoupling (220µF - 470µF):** Dipasang di jalur daya 5V dinamo untuk mencegah drop tegangan sesaat yang dapat memicu *Brownout Reset* pada ESP32.
4. **Kebutuhan Adaptor Power Supply:** Gunakan adaptor daya USB minimal **5V 2A**.
