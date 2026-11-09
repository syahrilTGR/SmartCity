# Project Configuration: Smart City ESP32

## Stack
- **ESP32 Firmware**: PlatformIO — root repo ini (`platformio.ini` di root, source di `src/`)
- **Framework**: Arduino (`esp32dev`)
- **Dokumentasi**: `docs/project_brief.md`

## Rules
- **Compile & upload firmware dijalankan user sendiri**, bukan oleh agent. Agent hanya edit kode firmware dan verifikasi sintaks jika diminta.
- **Jangan update dokumentasi (`docs/*.md`, `README.md`) sebelum user menyatakan pekerjaan sudah benar / diminta eksplisit.** Agent fokus merancang atau mengedit kode terlebih dahulu. Dokumentasi di-update setelah ada konfirmasi dari user agar tidak terjadi perubahan ganda atau pemborosan token.
- **Pin Definition Konsisten:** Seluruh pemetaan pin harus mengacu pada tabel pinout di `docs/project_brief.md` dan didefinisikan secara eksplisit di `src/main.cpp`.
- **Pemisahan File Dokumentasi (Modular Markdown):** Jika konten dokumentasi mulai membahas domain/topik spesifik yang berbeda (misalnya alokasi pin, analisis arus/kelistrikan, skenario simulator), pisahkan ke file `.md` baru yang terfokus di folder `docs/` (misal: `docs/pinout.md`, `docs/power_budget.md`). Hal ini bertujuan untuk menghemat token konteks, mencegah file membengkak (*bloated*), dan mempermudah referensi cepat.
- **Format Respon Chat (Hindari Sintaks LaTeX):** Jangan menggunakan sintaks LaTeX (seperti `$..$`, `\Omega`, `\le`, `\approx`, `\text{..}`) dalam jawaban chat karena tidak ter-render di antarmuka chat. Gunakan teks polos atau simbol Unicode biasa yang mudah dibaca langsung (contoh: "47 Ohm - 68 Ohm", "<= 60 mA", "Ω", "~", "<=").
- **Jangan Melakukan Git Commit Tanpa Permintaan:** Agent dilarang menjalankan perintah `git commit` secara inisiatif sendiri. Commit hanya dieksekusi jika diminta secara langsung dan eksplisit oleh user.
- Prioritaskan stabilitas kelistrikan dan proteksi mikrokontroler dari beban berlebih.

## Commands (Untuk Dijalankan User)
```bash
# Firmware (dari root repo ini)
pio run                      # Build / compile test
pio run -t upload            # Upload ke ESP32
pio device monitor           # Serial monitor (115200 baud)
```

## Arsitektur Singkat (Revisi 10-Port BNC & Motor Driver L298N, 9 Nov 2026)
1. **Penerangan Rumah (4 LED):** GPIO 2 (Soket [L]), keempat LED digabung, masing-masing R 100 Ohm, total sekitar 12 mA. Menyala saat malam hari (`isNight == true`).
2. **LPJ Jalur Atas (2 LED):** GPIO 14, 27, dipicu mandiri oleh IR Proximity 1 (GPIO 34) & IR Proximity 2 (GPIO 35), hanya bersiaga saat malam hari, off-delay 1 detik.
3. **LPJ Jalur Bawah (4 LED, Simulator Adaptif 10-Port):**
   - Master Enable: malam hari saja (`isLowerStreetArmed == isNight`). Saklar toggle bukan lagi pin GPIO, melainkan pemutus seri jalur 3.3V ke modul LDR (Soket [I]).
   - 8 port beban A1-D2 (GPIO 4, 25, 16, 17, 21, 22, 18, 19) dipindai tiap 40 ms (baca ganda pull-up/pull-down, analisis graf Union-Find + DFS). Kecerahan PWM mengikuti rantai seri: 1 lampu 100%, 2 lampu 50%, 3 lampu 33%, 4 lampu 25%.
   - Output lampu: Lampu 1 GPIO 26 (Soket [H]), Lampu 2 GPIO 12 (Soket [M]), Lampu 3 GPIO 33 (Soket [D] Pin 3), Lampu 4 GPIO 32 (Soket [D] Pin 1).
   - Deteksi korslet SRC+ ke SRC- murni dari analisis graf (kedua net jadi satu), bukan dari pin sense terpisah. Semua lampu bawah padam saat korslet.
4. **Driver Motor L298N (2x Dinamo DC 1-Arah):** GPIO 15 (Soket [J]) mengendalikan IN1+IN3. Default ON, bisa diatur via MQTT / Serial. Catu daya motor +5V switched lewat Soket [G].
5. **Traffic Light VKS:** GPIO 23 (R), GPIO 5 (Y), GPIO 13 (G), dipantau sinyal daya solar via Optocoupler PC817 pada GPIO 39 (VN).
6. **Catu Daya:** Baterai 18650 2S (7.4V) + BMS 2S -> Saklar Master -> Modul Step-Down 5V (MP1584EN).

