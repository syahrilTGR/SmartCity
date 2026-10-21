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
- Prioritaskan stabilitas kelistrikan dan proteksi mikrokontroler dari beban berlebih.

## Commands (Untuk Dijalankan User)
```bash
# Firmware (dari root repo ini)
pio run                      # Build / compile test
pio run -t upload            # Upload ke ESP32
pio device monitor           # Serial monitor (115200 baud)
```

## Arsitektur Singkat
1. **Penerangan Rumah (4 LED):** GPIO 18, 19, 21, 22.
2. **LPJ Jalur Atas (2 LED):** GPIO 14, 27 — dipicu mandiri 24/7 oleh IR Proximity 1 (GPIO 34) & IR Proximity 2 (GPIO 35).
3. **LPJ Jalur Bawah (4 LED):** 
   - Master Enable: LDR (GPIO 36 / VP, aktif malam hari).
   - Seri (2 LED: GPIO 26, 25): Dipicu oleh loop sensing BNC seri pada GPIO 4 (`INPUT_PULLUP`).
   - Paralel (2 LED: GPIO 33, 32): Dipicu oleh sensing BNC cabang 1 pada GPIO 16 dan cabang 2 pada GPIO 17 (`INPUT_PULLUP`).
4. **Dummy Turbin Angin:** Dinamo Tamiya 5V dengan saklar fisik manual (tanpa GPIO).
