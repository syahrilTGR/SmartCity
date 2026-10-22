# Pemetaan Pinout ESP32 (Smart City)

Dokumen ini memuat pemetaan pin GPIO mikrokontroler **ESP32 Dev Module** (`esp32dev`) untuk seluruh modul sensor, lampu miniatur, simulator anti-curang, dan saklar kontrol, lengkap dengan panduan penyambungan (*wiring*) untuk skematik Fritzing.

---

## 1. Tabel Alokasi Pin GPIO

| Pin GPIO | Mode Pin | Komponen Terhubung | Arah Sinyal | Keterangan & Catatan Fritzing |
| :---: | :---: | :--- | :---: | :--- |
| **GPIO 34** | `INPUT` | Sensor IR Proximity 1 | Masukan Digital | Input-Only pin, aktif 24/7 (Sinyal LOW saat mobil terdeteksi) |
| **GPIO 35** | `INPUT` | Sensor IR Proximity 2 | Masukan Digital | Input-Only pin, aktif 24/7 (Sinyal LOW saat mobil terdeteksi) |
| **GPIO 36 (VP)** | `ANALOG` | Sensor LDR | Masukan Analog | ADC1 Channel 0, mendeteksi siang/malam (Rangkaian pembagi tegangan R 10k) |
| **GPIO 39 (VN)** | `INPUT` | Sinyal Solar (Optocoupler) | Masukan Digital | Input-Only pin, membaca daya panel surya via PC817 (LOW saat ada sinar) |
| **GPIO 4** | `INPUT_PULLUP` | Sensing BNC Seri (Pasangan 1) | Masukan Digital | Lubang BNC 1-2. Terbaca LOW jika kabel BNC 1 terpasang ke GND |
| **GPIO 25** | `INPUT_PULLUP` | Sensing BNC Seri (Pasangan 2) | Masukan Digital | Lubang BNC 3-4. Terbaca LOW jika kabel BNC 2 terpasang ke GND (**Anti-Curang**) |
| **GPIO 16** | `INPUT_PULLUP` | Sensing BNC Paralel 1 | Masukan Digital | Lubang BNC Paralel 1 ke GND. Terbaca LOW saat kabel BNC terpasang |
| **GPIO 17** | `INPUT_PULLUP` | Sensing BNC Paralel 2 | Masukan Digital | Lubang BNC Paralel 2 ke GND. Terbaca LOW saat kabel BNC terpasang |
| **GPIO 15** | `INPUT_PULLUP` | Saklar Toggle Sensor | Masukan Digital | Saklar manual toggle ke GND untuk mengaktifkan/menonaktifkan otomasi sensor |
| **GPIO 14** | `OUTPUT` | LPJ Jalur Atas 1 | Keluaran Digital | Terhubung ke Anoda LED via Resistor 100 Ohm (Terpicu IR 1) |
| **GPIO 27** | `OUTPUT` | LPJ Jalur Atas 2 | Keluaran Digital | Terhubung ke Anoda LED via Resistor 100 Ohm (Terpicu IR 2) |
| **GPIO 26** | `OUTPUT` | 2 LED LPJ Seri Kiri Bawah | Keluaran Digital | Mengontrol 2 LED Seri sekaligus (diparalel fisik, masing-masing R 100 Ohm) |
| **GPIO 33** | `OUTPUT` | LPJ Kanan Bawah 1 | Keluaran Digital | Terhubung ke Anoda LED via Resistor 100 Ohm (Lampu Paralel 1) |
| **GPIO 32** | `OUTPUT` | LPJ Kanan Bawah 2 | Keluaran Digital | Terhubung ke Anoda LED via Resistor 100 Ohm (Lampu Paralel 2) |
| **GPIO 18** | `OUTPUT` | 2 LED Rumah (Rumah 1 & 2) | Keluaran Digital | Mengontrol Rumah 1 & 2 (diparalel fisik, masing-masing R 100 Ohm) |
| **GPIO 19** | `OUTPUT` | 2 LED Rumah (Rumah 3 & 4) | Keluaran Digital | Mengontrol Rumah 3 & 4 (diparalel fisik, masing-masing R 100 Ohm) |
| **GPIO 21** | *NC* | Bebas (*Reserved*) | - | Tidak terhubung (Tersedia untuk ekspansi masa depan) |
| **GPIO 22** | *NC* | Bebas (*Reserved*) | - | Tidak terhubung (Tersedia untuk ekspansi masa depan) |
| **GPIO 23** | `OUTPUT` | Traffic Light Merah | Keluaran Digital | Terhubung ke Pin R (Red) Modul VKS 5V |
| **GPIO 5** | `OUTPUT` | Traffic Light Kuning | Keluaran Digital | Terhubung ke Pin Y (Yellow) Modul VKS 5V |
| **GPIO 13** | `OUTPUT` | Traffic Light Hijau | Keluaran Digital | Terhubung ke Pin G (Green) Modul VKS 5V |
| *Non-GPIO* | *Hardware* | Saklar Power Utama | Switch SPST | Memutus jalur baterai 2S (7.4V) sebelum masuk modul Step-Down 5V |
| *Non-GPIO* | *Manual* | Dinamo Turbin Angin | Jalur Daya 5V | Motor DC Tamiya 130 + Saklar manual mekanik (bebas GPIO) |

---

## 2. Catatan Keamanan Alokasi Pin

1. **Bebas dari Masalah GPIO 2:**
   - Tidak menggunakan `GPIO 2`, sehingga lampu LED biru bawaan ESP32 tidak terganggu dan proses upload firmware tetap lancar.
2. **Kesesuaian Fitur Khusus Pin:**
   - `GPIO 34`, `35`, `36`, dan `39` adalah pin **Input-Only** (tidak memiliki fungsi output fisik). Alokasi sensor IR Proximity, LDR, dan Optocoupler pada pin ini sangat tepat dan menghemat pin output.
   - Sensor LDR dialokasikan pada **ADC1** (`GPIO 36`), sehingga tetap dapat membaca nilai analog secara akurat tanpa terganggu sekalipun fitur Wi-Fi ESP32 aktif di masa mendatang.
3. **Internal Pull-Up:**
   - Seluruh pin sensing simulator dan saklar toggle (`GPIO 4`, `GPIO 25`, `GPIO 16`, `GPIO 17`, `GPIO 15`) memanfaatkan resistor internal pull-up mikrokontroler (~45 kOhm), sehingga tidak memerlukan resistor pull-up eksternal.

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

### B. Rangkaian Simulator BNC Seri (Sistem 2 Pasangan Anti-Curang)
Mencegah kecurangan 1 kabel langsung dari lubang paling kiri ke paling kanan:
* **Pasangan 1 (Kabel BNC Seri Kiri):**
  `GND ESP32 ─────[ Lubang BNC 1 ] ───(Kabel A)─── [ Lubang BNC 2 ] ───── GPIO 4 ESP32`
* **Pasangan 2 (Kabel BNC Seri Kanan):**
  `GND ESP32 ─────[ Lubang BNC 3 ] ───(Kabel B)─── [ Lubang BNC 4 ] ───── GPIO 25 ESP32`
* *Logika Program:* Lampu seri hanya menyala jika **GPIO 4 == LOW DAN GPIO 25 == LOW**. Jika dicolok silang atau hanya 1 kabel panjang, lampu seri tetap mati!

### C. Rangkaian 2 LED LPJ Seri Kiri Bawah (Diparalel Fisik dari GPIO 26)
```text
                               ┌───[ Resistor 100 Ohm ]───[ Anoda (+) LED Seri 1 (-) ]───┐
                               │                                                         │
Pin GPIO 26 ESP32 ─────────────┤                                                         ├─── GND ESP32
                               │                                                         │
                               └───[ Resistor 100 Ohm ]───[ Anoda (+) LED Seri 2 (-) ]───┘
```

### D. Rangkaian Simulator BNC Paralel (2 Push Button / BNC Paralel)
```text
GND ESP32 ────┬────[ BNC / Tombol Paralel 1 ]──── GPIO 16 ESP32
              │
              └────[ BNC / Tombol Paralel 2 ]──── GPIO 17 ESP32
```

### E. Rangkaian Saklar Toggle Sensor ON/OFF
```text
GND ESP32 ────────[ Saklar Toggle SPST ]──────── GPIO 15 ESP32 (INPUT_PULLUP)
```
* Saklar ON (tertutup ke GND): GPIO 15 bernilai `LOW` -> Sensor aktif.
* Saklar OFF (terbuka): GPIO 15 bernilai `HIGH` -> Sensor non-aktif / mode manual.

### F. Rangkaian LED Rumah (2 LED per Pin GPIO)
* **Rumah 1 & 2 (GPIO 18):**
```text
                       ┌───[ Resistor 100 Ohm ]───[ Anoda (+) LED Rumah 1 (-) ]───┐
                       │                                                          │
Pin GPIO 18 ESP32 ─────┤                                                          ├─── GND ESP32
                       │                                                          │
                       └───[ Resistor 100 Ohm ]───[ Anoda (+) LED Rumah 2 (-) ]───┘
```
* **Rumah 3 & 4 (GPIO 19):**
```text
                       ┌───[ Resistor 100 Ohm ]───[ Anoda (+) LED Rumah 3 (-) ]───┐
                       │                                                          │
Pin GPIO 19 ESP32 ─────┤                                                          ├─── GND ESP32
                       │                                                          │
                       └───[ Resistor 100 Ohm ]───[ Anoda (+) LED Rumah 4 (-) ]───┘
```

### G. Rangkaian LPJ Lainnya (4 Unit LED Tunggal)
Untuk 2 LPJ Jalur Atas (GPIO 14, GPIO 27) dan 2 LPJ Kanan Bawah (GPIO 32, GPIO 33):
```text
Pin GPIO Terkait ───[ Resistor 100 Ohm ]───[ Anoda (+) LED (-) Katoda ]─── GND ESP32
```

### G. Rangkaian Sensor IR Proximity (2 Unit)
* **VCC:** Ke pin **3.3V** atau **5V** ESP32
* **GND:** Ke pin **GND** ESP32
* **OUT Sensor 1:** Ke pin **GPIO 34**
* **OUT Sensor 2:** Ke pin **GPIO 35**

### H. Rangkaian Traffic Light VKS & Optocoupler PC817
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
