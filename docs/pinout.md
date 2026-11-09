# Pemetaan Pinout ESP32 (Smart City)

Dokumen ini memuat pemetaan pin GPIO mikrokontroler **ESP32 Dev Module** (`esp32dev`) untuk seluruh modul sensor, lampu miniatur, simulator adaptif 10-port, dan saklar kontrol, lengkap dengan panduan penyambungan (*wiring*) untuk skematik Fritzing.

> Disinkronkan ke `src/main.cpp` (revisi 10-port BNC dan driver motor L298N, commit `e36c33c`) pada 9 Nov 2026. Kalau ada yang bertentangan, kode yang menang.

---

## 1. Tabel Alokasi Pin GPIO

### A. Sensor Masukan (Input-Only)

| Pin GPIO | Mode Pin | Komponen Terhubung | Arah Sinyal | Keterangan |
| :---: | :---: | :--- | :---: | :--- |
| **GPIO 34** | `INPUT` | Sensor IR Proximity 1 | Masukan Digital | Input-Only pin. Sinyal LOW saat mobil terdeteksi. Hanya bersiaga saat malam hari |
| **GPIO 35** | `INPUT` | Sensor IR Proximity 2 | Masukan Digital | Input-Only pin. Sinyal LOW saat mobil terdeteksi. Hanya bersiaga saat malam hari |
| **GPIO 36 (VP)** | `INPUT` | Modul Sensor LDR (LM393, Digital DO) | Masukan Digital | DO HIGH berarti malam (`isNight = true`), DO LOW berarti siang. VCC modul diputus saklar toggle seri, bukan lewat GPIO |
| **GPIO 39 (VN)** | `INPUT` | Sinyal Solar (Optocoupler) | Masukan Digital | Input-Only pin, membaca daya panel surya via PC817 (LOW saat ada sinar). Ada filter histeresis anti-flicker |

### B. Port Simulator BNC (8 Port Beban, I/O Dinamis)

Port-port ini dipindai ESP32 tiap 40 ms. Saat pemindaian, tiap pin hanya di-drive ke `OUTPUT LOW` atau dibiarkan `INPUT`, tidak pernah di-drive `HIGH` (proteksi korslet). Tiap jalur memakai resistor seri 150 Ohm tepat di belakang soket BNC.

| Pin GPIO | Port Panel | Pasangan Lampu | Soket JST | Keterangan |
| :---: | :---: | :---: | :---: | :--- |
| **GPIO 4** | A1 | Lampu 1 | Soket `[O]` Pin 1 | Kutub 1 Lampu 1 |
| **GPIO 25** | A2 | Lampu 1 | Soket `[O]` Pin 3 | Kutub 2 Lampu 1 |
| **GPIO 16** | B1 | Lampu 2 | Soket `[N]` Pin 1 (RX2) | Kutub 1 Lampu 2 |
| **GPIO 17** | B2 | Lampu 2 | Soket `[N]` Pin 3 (TX2) | Kutub 2 Lampu 2 |
| **GPIO 21** | C1 | Lampu 3 | Soket `[N]` Pin 2 | Kutub 1 Lampu 3 |
| **GPIO 22** | C2 | Lampu 3 | Soket `[K]` Pin 3 | Kutub 2 Lampu 3 |
| **GPIO 18** | D1 | Lampu 4 | Soket `[K]` Pin 1 | Kutub 1 Lampu 4 |
| **GPIO 19** | D2 | Lampu 4 | Soket `[K]` Pin 2 | Kutub 2 Lampu 4 |

Terminal sumber simulator: `SRC (+)` 3.3V lewat resistor 1 kOhm (Soket `[O]` Pin 2), `SRC (-)` GND (Soket `[O]` Pin 4).

### C. Output Lampu dan Motor

| Pin GPIO | Mode Pin | Komponen Terhubung | Soket JST | Keterangan |
| :---: | :---: | :---: | :---: | :--- |
| **GPIO 14** | `OUTPUT` | LPJ Jalur Atas 1 | Soket `[C]` Pin 1 | Terpicu IR 1, hanya malam hari. Resistor 100 Ohm |
| **GPIO 27** | `OUTPUT` | LPJ Jalur Atas 2 | Soket `[C]` Pin 3 | Terpicu IR 2, hanya malam hari. Resistor 100 Ohm |
| **GPIO 26** | `OUTPUT` (PWM) | LPJ Bawah 1 | Soket `[H]` | Lampu simulator 1, kecerahan PWM adaptif. Resistor 100 Ohm |
| **GPIO 12** | `OUTPUT` (PWM) | LPJ Bawah 2 | Soket `[M]` | Lampu simulator 2, kecerahan PWM adaptif. Resistor 100 Ohm |
| **GPIO 33** | `OUTPUT` (PWM) | LPJ Bawah 3 | Soket `[D]` Pin 3 | Lampu simulator 3, kecerahan PWM adaptif. Resistor 100 Ohm |
| **GPIO 32** | `OUTPUT` (PWM) | LPJ Bawah 4 | Soket `[D]` Pin 1 | Lampu simulator 4, kecerahan PWM adaptif. Resistor 100 Ohm |
| **GPIO 2** | `OUTPUT` | 4 LED Rumah (Rumah 1-4) | Soket `[L]` | Keempat LED diparalel fisik, masing-masing R 100 Ohm. Arus total sekitar 12 mA. Menyala saat malam hari |
| **GPIO 15** | `OUTPUT` (PWM) | Driver Motor L298N (IN1+IN3) | Soket `[J]` | Kendali 2 kanal dinamo DC 1 arah. Default ON, bisa diatur via MQTT / Serial |
| **GPIO 23** | `OUTPUT` | Traffic Light Merah | Soket `[P]` Pin 1 | Terhubung ke Pin R Modul VKS 5V |
| **GPIO 5** | `OUTPUT` | Traffic Light Kuning | Soket `[P]` Pin 2 | Terhubung ke Pin Y Modul VKS 5V |
| **GPIO 13** | `OUTPUT` | Traffic Light Hijau | Soket `[P]` Pin 3 | Terhubung ke Pin G Modul VKS 5V |

### D. Bukan GPIO

| Jalur | Fungsi |
| :--- | :--- |
| Saklar Power Utama (SPST) | Memutus jalur baterai 2S (7.4V) sebelum masuk modul Step-Down 5V |
| Saklar Toggle Otomasi | Seri di jalur VCC 3.3V ke modul LDR (Soket `[I]`). OFF berarti LDR mati fisik dan sistem membaca siang |
| Catu Daya Motor +5VS | Soket `[G]`, jalur +5V switched (saklar fisik kincir) ke modul driver L298N |

---

## 2. Catatan Keamanan Alokasi Pin

1. **Pin Strapping (GPIO 2, GPIO 12, GPIO 15):**
   - Ketiga pin ini dipakai untuk periferal internal yang statusnya terkontrol penuh saat *boot*: GPIO 2 untuk LED rumah, GPIO 12 untuk LPJ Bawah 2, GPIO 15 untuk driver motor L298N.
   - Seluruh port eksternal BNC (A1 s/d D2) hanya dialokasikan pada pin GPIO non-strapping, supaya ESP32 tidak gagal *boot* kalau siswa menancapkan kabel sebelum alat dinyalakan. Detailnya ada di `docs/matriks_logika_8port_bnc.md` bagian 2.
2. **Kesesuaian Fitur Khusus Pin:**
   - `GPIO 34`, `35`, `36`, dan `39` adalah pin **Input-Only** (tidak memiliki fungsi output fisik). Alokasi sensor IR Proximity, LDR, dan Optocoupler pada pin ini sangat tepat dan menghemat pin output.
   - Sensor LDR sekarang memakai modul digital DO (LM393) di `GPIO 36`, bukan pembagi tegangan analog. Modul ini kebal interferensi Wi-Fi karena keluarannya digital.
3. **Internal Pull-Up / Pull-Down:**
   - Selama pemindaian matriks, pin port BNC dibaca dua kali (pull-up lalu pull-down) untuk membedakan port mengambang, port ke `SRC (+)`, dan port ke `SRC (-)`. Tidak ada resistor pull-up eksternal di jalur port.
4. **Aturan Penggerak Pin (Wajib):**
   - Pin port BNC tidak pernah di-drive `HIGH`. Hanya `OUTPUT LOW` atau `INPUT` yang diizinkan saat pemindaian. Kalau pin di-drive `HIGH` lalu siswa mencolokkannya ke `SRC (-)`, arus puluhan mA akan mengalir langsung ke chip.
5. **Beban per Pin:**
   - Tiap LED memakai resistor 100 Ohm, arusnya sekitar 3 mA per LED. Empat LED rumah di GPIO 2 totalnya sekitar 12 mA, masih di bawah batas aman per pin (12-20 mA).

---

## 3. Panduan Skema Wiring untuk Fritzing

### A. Modul Sensor LDR Digital (LM393)
Modul LDR yang dipakai sekarang adalah modul komparator LM393 dengan output digital DO, bukan LDR lepasan dengan pembagi tegangan.
```text
3.3V ESP32 ────[ Saklar Toggle Seri ]──── VCC Modul LDR
GND ESP32 ─────────────────────────────── GND Modul LDR
GPIO 36 (VP) ESP32 ────────────────────── DO Modul LDR
```
* DO `HIGH` berarti gelap / malam hari. DO `LOW` berarti terang / siang hari.
* Saklar toggle OFF berarti modul mati fisik, DO terbaca siang, dan seluruh otomasi lampu padam.

### B. Simulator BNC 10-Port (Matriks Adaptif)
Delapan port beban dan dua terminal sumber. Detail lengkap (algoritma baca ganda, analisis graf, tabel skenario) ada di `docs/matriks_logika_8port_bnc.md`.
```text
SRC (+) ───[ Resistor 1 kOhm ]─── Terminal SRC+ (Soket [O] Pin 2)
SRC (-) ───────────────────────── Terminal SRC- (Soket [O] Pin 4, GND)

A1 (GPIO 4) ───( Lampu 1 )─── A2 (GPIO 25)
B1 (GPIO 16) ───( Lampu 2 )─── B2 (GPIO 17)
C1 (GPIO 21) ───( Lampu 3 )─── C2 (GPIO 22)
D1 (GPIO 18) ───( Lampu 4 )─── D2 (GPIO 19)
```
* Tiap jalur port memakai resistor seri 150 Ohm di belakang soket BNC.
* Lampu simulator menyala sesuai rangkaian yang dirangkai siswa, dengan kecerahan PWM 100% / 50% / 33% / 25% untuk rantai 1 / 2 / 3 / 4 lampu.

### C. Rangkaian 4 LED LPJ Jalur Bawah (Output Mandiri)
Tiap lampu punya pin output sendiri, bukan lagi 2 LED yang diparalel dari satu pin.
```text
Pin GPIO 26 ───[ Resistor 100 Ohm ]───[ Anoda (+) LED LPJ Bawah 1 (-) ]─── GND
Pin GPIO 12 ───[ Resistor 100 Ohm ]───[ Anoda (+) LED LPJ Bawah 2 (-) ]─── GND
Pin GPIO 33 ───[ Resistor 100 Ohm ]───[ Anoda (+) LED LPJ Bawah 3 (-) ]─── GND
Pin GPIO 32 ───[ Resistor 100 Ohm ]───[ Anoda (+) LED LPJ Bawah 4 (-) ]─── GND
```

### D. Rangkaian LED Rumah (4 LED di Satu Pin GPIO 2)
```text
                       ┌───[ Resistor 100 Ohm ]───[ Anoda (+) LED Rumah 1 (-) ]───┐
                       │                                                          │
                       ├───[ Resistor 100 Ohm ]───[ Anoda (+) LED Rumah 2 (-) ]───┤
Pin GPIO 2 ESP32 ──────┤                                                          ├─── GND ESP32
                       ├───[ Resistor 100 Ohm ]───[ Anoda (+) LED Rumah 3 (-) ]───┤
                       │                                                          │
                       └───[ Resistor 100 Ohm ]───[ Anoda (+) LED Rumah 4 (-) ]───┘
```

### E. Rangkaian LPJ Jalur Atas (2 Unit LED Tunggal)
```text
Pin GPIO 14 ───[ Resistor 100 Ohm ]───[ Anoda (+) LED LPJ Atas 1 (-) ]─── GND ESP32
Pin GPIO 27 ───[ Resistor 100 Ohm ]───[ Anoda (+) LED LPJ Atas 2 (-) ]─── GND ESP32
```
* Kedua lampu hanya bersiaga saat malam hari dan dipicu sensor IR masing-masing, dengan off-delay 1 detik.

### F. Rangkaian Driver Motor L298N
```text
GPIO 15 ESP32 ───────────────────── IN1 + IN3 Modul L298N (Soket [J] Pin 1)
GND ESP32 ───────────────────────── GND Modul L298N (Soket [J] Pin 2)
+5V Switched ────────────────────── Catu daya motor modul L298N (Soket [G])
```
* GPIO 15 mengendalikan 2 kanal dinamo DC 1 arah sekaligus (ON/OFF dan PWM). Default motor aktif, bisa diubah lewat MQTT / Serial.

### G. Rangkaian Saklar Toggle Otomasi (Seri VCC LDR)
```text
3.3V ESP32 ────────[ Saklar Toggle SPST ]──────── VCC Modul LDR (Soket [I] Pin 1)
DO Modul LDR ──────────────────────────────────── GPIO 36 ESP32 (Soket [I] Pin 2)
```
* Saklar ON: modul LDR mendapat daya, sistem membaca siang/malam dari DO.
* Saklar OFF: modul mati fisik, sistem membaca siang, seluruh otomasi lampu padam.

### H. Rangkaian Sensor IR Proximity (2 Unit)
* **VCC:** Ke pin **3.3V** atau **5V** ESP32
* **GND:** Ke pin **GND** ESP32
* **OUT Sensor 1:** Ke pin **GPIO 34**
* **OUT Sensor 2:** Ke pin **GPIO 35**

### I. Rangkaian Traffic Light VKS & Optocoupler PC817
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
