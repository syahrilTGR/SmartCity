# Pemetaan Pinout ESP32 (Smart City)

Dokumen ini memuat pemetaan pin GPIO mikrokontroler **ESP32 Dev Module** (`esp32dev`) untuk seluruh modul sensor, lampu miniatur, dan simulator.

---

## 1. Tabel Alokasi Pin GPIO

| Pin GPIO | Mode Pin | Komponen Terhubung | Arah Sinyal | Keterangan & Catatan Teknis |
| :---: | :---: | :--- | :---: | :--- |
| **GPIO 34** | `INPUT` | Sensor IR Proximity 1 | Masukan Digital | Input-Only pin, aktif 24/7 mendeteksi mobil LPJ Atas 1 (Active LOW) |
| **GPIO 35** | `INPUT` | Sensor IR Proximity 2 | Masukan Digital | Input-Only pin, aktif 24/7 mendeteksi mobil LPJ Atas 2 (Active LOW) |
| **GPIO 36 (VP)** | `ANALOG` | Sensor LDR | Masukan Analog | ADC1 Channel 0, mendeteksi siklus siang/malam (aman saat Wi-Fi on) |
| **GPIO 4** | `INPUT_PULLUP` | Sensing BNC Seri | Masukan Digital | Terhubung ke loop 4 female BNC. Terbaca `LOW` saat seluruh BNC tertutup |
| **GPIO 16** | `INPUT_PULLUP` | Sensing BNC Paralel 1 | Masukan Digital | Terhubung ke BNC Cabang 1. Terbaca `LOW` saat tertutup ke GND |
| **GPIO 17** | `INPUT_PULLUP` | Sensing BNC Paralel 2 | Masukan Digital | Terhubung ke BNC Cabang 2. Terbaca `LOW` saat tertutup ke GND |
| **GPIO 14** | `OUTPUT` | LPJ Jalur Atas 1 | Keluaran Digital | Lampu jalan atas 1, terpicu IR Proximity 1 |
| **GPIO 27** | `OUTPUT` | LPJ Jalur Atas 2 | Keluaran Digital | Lampu jalan atas 2, terpicu IR Proximity 2 |
| **GPIO 26** | `OUTPUT` | LPJ Kiri Bawah 1 | Keluaran Digital | Lampu seri 1 (nyala jika malam & loop seri BNC tertutup) |
| **GPIO 25** | `OUTPUT` | LPJ Kiri Bawah 2 | Keluaran Digital | Lampu seri 2 (nyala jika malam & loop seri BNC tertutup) |
| **GPIO 33** | `OUTPUT` | LPJ Kanan Bawah 1 | Keluaran Digital | Lampu paralel 1 (nyala jika malam & BNC cabang 1 tertutup) |
| **GPIO 32** | `OUTPUT` | LPJ Kanan Bawah 2 | Keluaran Digital | Lampu paralel 2 (nyala jika malam & BNC cabang 2 tertutup) |
| **GPIO 18** | `OUTPUT` | LED Rumah 1 | Keluaran Digital | Pencahayaan Miniatur Rumah 1 |
| **GPIO 19** | `OUTPUT` | LED Rumah 2 | Keluaran Digital | Pencahayaan Miniatur Rumah 2 |
| **GPIO 21** | `OUTPUT` | LED Rumah 3 | Keluaran Digital | Pencahayaan Miniatur Rumah 3 |
| **GPIO 22** | `OUTPUT` | LED Rumah 4 | Keluaran Digital | Pencahayaan Miniatur Rumah 4 |
| *Non-GPIO* | *Manual* | Dinamo Turbin Angin | Jalur Daya 5V | Motor DC Tamiya 130 + Saklar manual mekanik (bebas GPIO) |

---

## 2. Catatan Keamanan Alokasi Pin

1. **Bebas Strapping Pin Berbahaya:**
   - Tidak menggunakan pin strapping kritis yang dapat mengganggu proses *booting* atau flashing firmware (seperti `GPIO 0`, `GPIO 2`, `GPIO 12`, dan `GPIO 15`).
2. **Kesesuaian Fitur Khusus Pin:**
   - `GPIO 34`, `35`, dan `36` adalah pin **Input-Only** (tidak memiliki fungsi output fisik). Alokasi sensor IR Proximity dan LDR pada pin ini sangat tepat dan menghemat pin output.
   - Sensor LDR dialokasikan pada **ADC1** (`GPIO 36`), sehingga tetap dapat membaca nilai analog secara akurat tanpa terganggu sekalipun fitur Wi-Fi ESP32 aktif di masa mendatang.
3. **Internal Pull-Up:**
   - Pin sensing BNC (`GPIO 4`, `GPIO 16`, `GPIO 17`) memanfaatkan resistor internal pull-up mikrokontroler ($\approx 45\ \text{k}\Omega$), sehingga tidak membutuhkan resistor eksternal tambahan pada soket BNC.
