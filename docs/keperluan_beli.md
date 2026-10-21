# Daftar Keperluan Komponen & Belanja

Dokumen ini memuat daftar belanja komponen yang diperlukan untuk proyek Smart City, mencakup modul **Traffic Light (Opsi A Terpilih dan Opsi B Alternatif)** serta komponen sistem utama ESP32.

---

## 1. Komponen Traffic Light — OPSI A (Terpilih: ESP32 + Optocoupler PC817)

*Opsi utama yang akan dibeli dan dirakit karena hemat komponen, terintegrasi dengan ESP32, dan durasi lampu dapat diprogram.*

| No | Komponen | Spesifikasi / Tipe | Jumlah Rekomendasi | Keterangan & Catatan Toko |
| :-: | :--- | :--- | :-: | :--- |
| 1 | **Modul Traffic Light VKS** | Modul LED 5V (Pin: R, Y, G, GND) | 1 unit | Modul lampu lalu lintas miniatur 3 warna |
| 2 | **Panel Surya Mini 5V** | 5V (100mA – 500mA) | 1 unit | Sensor pemanen daya matahari |
| 3 | **IC Optocoupler PC817** | PC817 (Paket DIP-4) | 1 – 2 buah | Pengaman isolasi sinyal panel surya ke ESP32 |
| 4 | **Resistor 330 Ohm** | 330 Ω (atau 470 Ω), 1/4 Watt | 2 buah | Pembatas arus LED internal optocoupler |
| 5 | **Resistor 10 kOhm** | 10k Ω, 1/4 Watt | 2 – 5 buah | Pull-up sinyal optocoupler ke 3.3V (dan pembagi tegangan LDR) |

---

## 2. Komponen Traffic Light — OPSI B (Arsip / Alternatif Hardware: NE555 + CD4017)

*Disimpan sebagai referensi alternatif jika ingin merakit modul lampu lalu lintas yang bekerja murni secara analog/diskrit tanpa ESP32.*

| No | Komponen | Spesifikasi / Tipe | Jumlah | Keterangan |
| :-: | :--- | :--- | :-: | :--- |
| 1 | **Modul Traffic Light VKS** | Modul LED 5V (Pin: R, Y, G, GND) | 1 unit | Modul lampu lalu lintas miniatur 3 warna |
| 2 | **Panel Surya Mini 5V** | 5V (200mA – 500mA) | 1 unit | Sumber daya mandiri |
| 3 | **IC Timer NE555** | NE555P / LM555 (Paket DIP-8) | 1 – 2 buah | Pembangkit clock pulsa sekuensial |
| 4 | **IC Decade Counter CD4017** | CD4017BE / HCF4017 (DIP-16) | 1 – 2 buah | Pencacah urutan sekuensial 3 warna |
| 5 | **Socket IC DIP-8** | 8 Pin (Lebar standar) | 1 buah | Pelindung IC 555 saat penyolderan |
| 6 | **Socket IC DIP-16** | 16 Pin (Lebar standar) | 1 buah | Pelindung IC 4017 saat penyolderan |
| 7 | **Potensiometer / Trimpot** | 100 kOhm (Trimpot 3296W / Potensio Mono) | 1 buah | Pengatur kecepatan transisi lampu lalu lintas |
| 8 | **Resistor 10 kOhm** | 10k Ω, 1/4 Watt | 2 buah | Timing pin 7 NE555 |
| 9 | **Kapasitor Elektrolit 10 µF** | 10 µF (16V / 25V / 50V) | 2 buah | Penentu durasi waktu timing NE555 |
| 10 | **Kapasitor Keramik 10 nF** | 10 nF (Kode angka: **103**) | 2 buah | Filter noise pin 5 NE555 |
| 11 | **Kapasitor Elektrolit 470 µF – 1000 µF** | 470 µF atau 1000 µF (16V / 25V) | 1 buah | Penstabil tegangan panel surya |

---

## 3. Komponen Sistem Utama Smart City (ESP32)

| No | Komponen | Spesifikasi / Tipe | Jumlah | Keterangan |
| :-: | :--- | :--- | :-: | :--- |
| 1 | **Resistor 100 Ohm** | 100 Ω, 1/4 Watt | 15 buah | Pembatas arus LED Rumah & LPJ (aman & dingin) |
| 2 | **Sensor IR Proximity** | Modul Obstacle Avoidance (FC-51 / KY-032) | 2 unit | Deteksi mobil jalur atas 1 & 2 (Active LOW) |
| 3 | **Sensor Cahaya LDR** | Fotoresistor LDR 5mm | 1 buah | Pendeteksi kondisi siang/malam |
| 4 | **Dioda Flyback 1N4007** | 1N4007 (1A 1000V) | 1 – 2 buah | Proteksi lonjakan induktif dinamo Tamiya |
| 5 | **Kapasitor Decoupling 220 µF – 470 µF** | Elektrolit 16V / 25V | 1 buah | Peredam drop tegangan saat dinamo dinyalakan |
| 6 | **Saklar Manual On/Off** | Toggle Switch / Saklar Geser / Rocker | 1 buah | Saklar fisik manual dinamo kincir angin 5V |
| 7 | **Soket Female BNC & Kabel BNC** | Soket BNC Chassis / PCB + Kabel BNC | 4 pasang | Media simulator rangkaian seri & paralel |
| 8 | **Baterai 18650 & Holder** | Baterai Li-Ion 3.7V + Holder 2 Slot Paralel | 2 unit | Catu daya portabel miniatur Smart City |
| 9 | **Modul Step-Up 5V** | MT3608 (2A) atau Modul Power Bank IP5306 | 1 unit | Menaikkan daya baterai 3.7V ke 5V stabil |
