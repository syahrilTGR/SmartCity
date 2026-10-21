# Pemetaan Pinout ESP32 (Smart City)

Dokumen ini memuat pemetaan pin GPIO mikrokontroler **ESP32 Dev Module** (`esp32dev`) untuk seluruh modul sensor, lampu miniatur, dan simulator, lengkap dengan panduan penyambungan (*wiring*) untuk skematik Fritzing.

---

## 1. Tabel Alokasi Pin GPIO

| Pin GPIO | Mode Pin | Komponen Terhubung | Arah Sinyal | Keterangan & Catatan Fritzing |
| :---: | :---: | :--- | :---: | :--- |
| **GPIO 34** | `INPUT` | Sensor IR Proximity 1 | Masukan Digital | Input-Only pin, aktif 24/7 (Sinyal LOW saat mobil terdeteksi) |
| **GPIO 35** | `INPUT` | Sensor IR Proximity 2 | Masukan Digital | Input-Only pin, aktif 24/7 (Sinyal LOW saat mobil terdeteksi) |
| **GPIO 36 (VP)** | `ANALOG` | Sensor LDR | Masukan Analog | ADC1 Channel 0, mendeteksi siang/malam (Rangkaian pembagi tegangan R 10k) |
| **GPIO 39 (VN)** | `INPUT` | Sinyal Solar (Optocoupler) | Masukan Digital | Input-Only pin, membaca daya panel surya via PC817 (LOW saat ada sinar) |
| **GPIO 4** | `INPUT_PULLUP` | Sensing BNC Seri | Masukan Digital | Di Fritzing: 2 Push Button berantai seri ke GND (Terbaca LOW jika keduanya ditekan) |
| **GPIO 16** | `INPUT_PULLUP` | Sensing BNC Paralel 1 | Masukan Digital | Di Fritzing: Push Button Paralel 1 ke GND (Terbaca LOW saat ditekan) |
| **GPIO 17** | `INPUT_PULLUP` | Sensing BNC Paralel 2 | Masukan Digital | Di Fritzing: Push Button Paralel 2 ke GND (Terbaca LOW saat ditekan) |
| **GPIO 14** | `OUTPUT` | LPJ Jalur Atas 1 | Keluaran Digital | Terhubung ke Anoda LED via Resistor 100 Ohm (Terpicu IR 1) |
| **GPIO 27** | `OUTPUT` | LPJ Jalur Atas 2 | Keluaran Digital | Terhubung ke Anoda LED via Resistor 100 Ohm (Terpicu IR 2) |
| **GPIO 26** | `OUTPUT` | LPJ Kiri Bawah 1 | Keluaran Digital | Terhubung ke Anoda LED via Resistor 100 Ohm (Lampu Seri 1) |
| **GPIO 25** | `OUTPUT` | LPJ Kiri Bawah 2 | Keluaran Digital | Terhubung ke Anoda LED via Resistor 100 Ohm (Lampu Seri 2) |
| **GPIO 33** | `OUTPUT` | LPJ Kanan Bawah 1 | Keluaran Digital | Terhubung ke Anoda LED via Resistor 100 Ohm (Lampu Paralel 1) |
| **GPIO 32** | `OUTPUT` | LPJ Kanan Bawah 2 | Keluaran Digital | Terhubung ke Anoda LED via Resistor 100 Ohm (Lampu Paralel 2) |
| **GPIO 18** | `OUTPUT` | LED Rumah 1 | Keluaran Digital | Terhubung ke Anoda LED via Resistor 100 Ohm |
| **GPIO 19** | `OUTPUT` | LED Rumah 2 | Keluaran Digital | Terhubung ke Anoda LED via Resistor 100 Ohm |
| **GPIO 21** | `OUTPUT` | LED Rumah 3 | Keluaran Digital | Terhubung ke Anoda LED via Resistor 100 Ohm |
| **GPIO 22** | `OUTPUT` | LED Rumah 4 | Keluaran Digital | Terhubung ke Anoda LED via Resistor 100 Ohm |
| **GPIO 23** | `OUTPUT` | Traffic Light Merah | Keluaran Digital | Terhubung ke Pin R (Red) Modul VKS 5V |
| **GPIO 5** | `OUTPUT` | Traffic Light Kuning | Keluaran Digital | Terhubung ke Pin Y (Yellow) Modul VKS 5V |
| **GPIO 13** | `OUTPUT` | Traffic Light Hijau | Keluaran Digital | Terhubung ke Pin G (Green) Modul VKS 5V |
| *Non-GPIO* | *Manual* | Dinamo Turbin Angin | Jalur Daya 5V | Motor DC Tamiya 130 + Saklar manual mekanik (bebas GPIO) |

---

## 2. Catatan Keamanan Alokasi Pin

1. **Bebas Strapping Pin Berbahaya:**
   - Tidak menggunakan pin strapping kritis yang dapat mengganggu proses *booting* atau flashing firmware (seperti `GPIO 0`, `GPIO 2`, `GPIO 12`, dan `GPIO 15`).
2. **Kesesuaian Fitur Khusus Pin:**
   - `GPIO 34`, `35`, dan `36` adalah pin **Input-Only** (tidak memiliki fungsi output fisik). Alokasi sensor IR Proximity dan LDR pada pin ini sangat tepat dan menghemat pin output.
   - Sensor LDR dialokasikan pada **ADC1** (`GPIO 36`), sehingga tetap dapat membaca nilai analog secara akurat tanpa terganggu sekalipun fitur Wi-Fi ESP32 aktif di masa mendatang.
3. **Internal Pull-Up:**
   - Pin sensing simulator (`GPIO 4`, `GPIO 16`, `GPIO 17`) memanfaatkan resistor internal pull-up mikrokontroler (~45 kOhm), sehingga tidak memerlukan resistor pull-up eksternal pada tombol.

---

## 3. Panduan Skema Wiring untuk Fritzing

### A. Rangkaian Sensor LDR (Pembagi Tegangan)
Jika menggunakan komponen LDR lepasan (2 kaki):
```text
3.3V ESP32 ────[ Kaki 1 LDR ]
                    │
               [ Kaki 2 LDR ] ───┬─── Masuk ke GPIO 36 (VP) ESP32
                                 │
                         [ Resistor 10k Ohm ]
                                 │
                             GND ESP32
```

### B. Rangkaian Simulator BNC Seri (2 Push Button Seri)
Untuk mensimulasikan karakteristik rangkaian seri di Fritzing:
```text
GND ESP32 ─────[ Tombol Seri 1 ]─────[ Tombol Seri 2 ]───── GPIO 4 ESP32
```
* Sinyal `LOW` (tersambung ke GND) hanya tercapai jika **KEDUA tombol dalam kondisi tertutup/ditekan**.
* Jika salah satu tombol terbuka/dilepas, jalur putus dan GPIO 4 membaca `HIGH`.

### C. Rangkaian Simulator BNC Paralel (2 Push Button Paralel)
Untuk mensimulasikan karakteristik rangkaian paralel di Fritzing:
```text
GND ESP32 ────┬────[ Tombol Paralel 1 ]──── GPIO 16 ESP32
              │
              └────[ Tombol Paralel 2 ]──── GPIO 17 ESP32
```
* Masing-masing tombol berdiri sendiri menghubungkan pin ke GND saat ditekan.

### D. Rangkaian Tiap Lampu LED (10 Unit LED)
Seluruh 10 LED (4 Rumah, 2 LPJ Atas, 4 LPJ Bawah) memiliki jalur identik:
```text
Pin GPIO Terkait ───[ Resistor 100 Ohm ]───[ Anoda (+) LED (-) Katoda ]─── GND ESP32
```

### E. Rangkaian Sensor IR Proximity (2 Unit)
* **VCC:** Ke pin **3.3V** atau **5V** ESP32
* **GND:** Ke pin **GND** ESP32
* **OUT Sensor 1:** Ke pin **GPIO 34**
* **OUT Sensor 2:** Ke pin **GPIO 35**

### F. Rangkaian Traffic Light VKS & Optocoupler PC817 (Opsi Terpilih)
1. **Modul Traffic Light VKS 5V:**
   * **Pin R (Merah):** Ke pin **GPIO 23**
   * **Pin Y (Kuning):** Ke pin **GPIO 5**
   * **Pin G (Hijau):** Ke pin **GPIO 13**
   * **Pin GND:** Ke pin **GND** ESP32
2. **Optocoupler PC817 (Deteksi Sinar Matahari):**
   * **Pin 1 (Anoda):** Ke Panel Surya (+) via Resistor 330 Ohm
   * **Pin 2 (Katoda):** Ke Panel Surya (-)
   * **Pin 3 (Emitor):** Ke pin **GND** ESP32
   * **Pin 4 (Kolektor):** Ke pin **GPIO 39 (VN)** ESP32, ditambah Resistor 10 kOhm ke pin **3.3V** ESP32 (Pull-Up)
