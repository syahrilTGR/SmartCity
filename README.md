# Smart City Miniatur Diorama (ESP32 + Simulator BNC 10-Port)

Firmware, desain hardware, dan dashboard monitoring berbasis **ESP32** untuk miniatur kota pintar (*Smart City*) yang menggabungkan otomasi pencahayaan, tenaga surya, turbin angin, serta simulator edukasi rangkaian listrik seri-paralel 10-port BNC.

---

## Fitur Utama

1. **Simulator Rangkaian Listrik Adaptif 10-Port BNC (LPJ Jalur Bawah)**
   * Menggunakan 8 port beban (`A1–D2`) dan 2 port sumber (`SRC+` 3.3V via resistor 1 kOhm dan `SRC-` GND).
   * Pemindaian matriks sinkron (*Synchronous Dual-Read*) setiap 40 ms dengan analisis graf **Union-Find + DFS**.
   * Kecerahan PWM 4 lampu jalan (GPIO 26, 12, 33, 32) menyesuaikan topologi rangkaian secara otomatis:
     * **1 Lampu / Paralel Penuh:** 100% (PWM 255)
     * **2 Lampu Seri:** 50% (PWM 128)
     * **3 Lampu Seri:** 33% (PWM 85)
     * **4 Lampu Seri:** 25% (PWM 64)
     * **Terbuka / Korslet Beban:** 0% (Padam)
   * Dilengkapi deteksi korsleting `SRC+` ke `SRC-` secara otomatis dan aktif 24/7 (siang maupun malam) agar mudah diuji kapan saja.

2. **Penerangan Rumah & Saklar Toggle 2 Mode**
   * 4 LED rumah digabung pada **GPIO 2** (Soket `[L]`).
   * Saklar toggle dipasang seri di jalur **GND modul LDR** (VCC 3.3V permanen):
     * **Saklar ON (Mode Otomatis):** Sensor LDR (GPIO 36) mendeteksi terang/gelap (siang = rumah & LPJ Atas padam, malam = menyala).
     * **Saklar OFF (Mode Mainan Biasa):** Jalur GND LDR terputus sehingga pull-up internal menarik pin DO ke `HIGH`, membuat **lampu rumah menyala terus** dan LPJ Atas tetap interaktif tanpa terpengaruh cahaya ruangan.

3. **LPJ Jalur Atas Otomatis (IR Proximity)**
   * 2 tiang lampu (GPIO 14 & 27) dipicu secara mandiri oleh Sensor IR 1 (GPIO 34) & IR 2 (GPIO 35) saat malam hari / mode mainan, dengan *off-delay* 1 detik.

4. **Turbin Angin via Driver Motor L298N**
   * Mengendalikan 2 dinamo DC 1-arah pada **GPIO 15** (Soket `[J]`, `IN1 + IN3`).
   * **Logika Starter Cerdas:** Dorongan awal 100% PWM (255) selama 300 ms untuk mengatasi torsi diam, lalu otomatis turun ke **Level 4 (80% / PWM 204)** untuk putaran operasional yang stabil. Dapat di-toggle via MQTT atau ketik `m` di Serial Monitor.

5. **Traffic Light Tenaga Surya (VKS 5V + Optocoupler PC817)**
   * Membaca daya panel surya secara terisolasi melalui Optocoupler PC817 pada **GPIO 39** dengan filter histeresis anti-flicker.
   * **Siang (Daya Surya Aktif):** Siklus normal Merah (5 dtk) -> Kuning (2 dtk) -> Hijau (4 dtk).
   * **Malam (Daya Surya Padam):** Mode siaga kota (*Flashing Yellow*).

6. **Wi-Fi Non-Blocking Penuh, OTA, & Dashboard Web MQTT**
   * **0 Detik Delay Boot:** Proses koneksi Wi-Fi berjalan di latar belakang sehingga seluruh fungsi diorama langsung hidup saat power ON. Jika Wi-Fi tersimpan tidak merespons dalam 10 detik, portal konfigurasi **`SmartCity-AP`** otomatis aktif tanpa menghambat `loop()`.
   * Mendukung **ArduinoOTA** dan reset kredensial Wi-Fi via Serial Monitor (ketik `r`).
   * Dashboard web statis ([web/index.html](web/index.html)) terhubung melalui MQTT WebSocket (`wss://broker.emqx.io:8084/mqtt`) serta dilengkapi mode **Demo Simulator** offline.

---

## Struktur Repositori

```text
├── platformio.ini          # Konfigurasi PlatformIO & daftar environment uji
├── src/
│   ├── main.cpp            # Firmware utama Smart City terintegrasi
│   ├── test_bnc.cpp        # Uji matriks simulator 10-port BNC
│   ├── test_motor.cpp      # Uji PWM & kickstart driver motor L298N
│   ├── test_ldr.cpp        # Uji sensor cahaya LDR digital (LM393)
│   ├── test_ir.cpp         # Uji sensor IR Proximity 1 & 2
│   ├── test_opto.cpp       # Uji Optocoupler PC817 & Traffic Light
│   └── test_toggle.cpp     # Uji saklar toggle
├── web/
│   └── index.html          # Dashboard monitoring real-time (MQTT & Demo)
└── docs/                   # Dokumentasi teknis modular (pinout, PCB, kelistrikan)
```

---

## Panduan Build & Upload (PlatformIO)

Pastikan [PlatformIO CLI](https://platformio.org/) atau ekstensi PlatformIO IDE sudah terpasang.

### 1. Firmware Utama (via USB)
```bash
pio run -e esp32dev -t upload
pio device monitor -b 115200
```

### 2. Upload Nirkabel (ArduinoOTA)
Ganti `<IP_ESP32>` dengan alamat IP yang tampil pada Serial Monitor:
```bash
pio run -e esp32dev -t upload --upload-port <IP_ESP32>
```

### 3. Firmware Pengujian Per-Modul
```bash
pio run -e test_bnc -t upload && pio device monitor    # Uji matriks BNC 10-port
pio run -e test_motor -t upload && pio device monitor  # Uji dinamo L298N
pio run -e test_ldr -t upload && pio device monitor    # Uji sensor LDR LM393
pio run -e test_ir -t upload && pio device monitor     # Uji sensor IR 1 & 2
pio run -e test_opto -t upload && pio device monitor   # Uji solar optocoupler & TL
```

### Dependensi Library
* `tzapu/WiFiManager @ ^2.0.17`
* `knolleary/PubSubClient @ ^2.8`
* `bblanchon/ArduinoJson @ ^7.3.0`

---

## Dokumentasi Teknis Lengkap

Detail spesifikasi perangkat keras dipisahkan per domain di folder `docs/`:
* **[docs/project_brief.md](docs/project_brief.md)** — Ringkasan arsitektur sistem dan logika operasi setiap subsistem.
* **[docs/pinout.md](docs/pinout.md)** — Pemetaan lengkap 25 GPIO ESP32 dan skema *wiring* rangkaian.
* **[docs/pcb_wiring.md](docs/pcb_wiring.md)** — Panduan koneksi soket JST pada PCB Matrix.
* **[docs/matriks_logika_8port_bnc.md](docs/matriks_logika_8port_bnc.md)** — Penjelasan algoritma *Synchronous Dual-Read*, graf Union-Find + DFS, dan skenario uji BNC.
* **[docs/power_budget.md](docs/power_budget.md)** — Analisis catu daya baterai 2S, BMS, step-down 5V, dan perhitungan arus LED/GPIO.
