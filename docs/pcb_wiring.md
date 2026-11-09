# Panduan Wiring PCB Matrix & Pemetaan Konektor JST (Molex)

Dokumen ini memuat panduan tata letak (*layout*) dan penyolderan pada **PCB Matrix (Perfboard)** dengan orientasi mikrokontroler **ESP32 Dev Module (30-pin)**, dikelompokkan secara fisik antara **Sisi Atas** dan **Sisi Bawah**, serta pemetaan soket **Molex JST-XH 2.54mm (2-Pin, 3-Pin, 4-Pin)**.

> Disinkronkan ke `src/main.cpp` (revisi 10-port BNC dan driver motor L298N, commit `e36c33c`) dan diagram `docs/pcb_matrix_layout_10port.svg` pada 9 Nov 2026. Kalau ada yang bertentangan, kode dan diagram SVG yang menang.

---

> [!TIP]
> **Diagram Visual Vektor Grid (A-Y × 1-29):**
> Denah visual yang berlaku adalah berkas revisi 10-port:
> 👉 **[docs/pcb_matrix_layout_10port.svg](pcb_matrix_layout_10port.svg)**

---

## 1. Orientasi Fisik & Mekanik PCB Matrix

* **Dimensi Grid Lubang:** **Kolom A – Y (25 Lubang)** × **Baris 1 – 29 (29 Lubang)**.
* **4 Lubang Baut / Standoff (2×2 Lubang Bebas di Tiap Pojokan):**
  * **Pojok Kiri Atas :** Kolom **A – B**, Baris **1 – 2**
  * **Pojok Kanan Atas :** Kolom **X – Y**, Baris **1 – 2**
  * **Pojok Kiri Bawah :** Kolom **A – B**, Baris **28 – 29**
  * **Pojok Kanan Bawah :** Kolom **X – Y**, Baris **28 – 29**
* **Aturan Spasi Antar-Soket JST:** Minimal menyisakan **1 lubang kosong (2.54mm)** di antara dua soket JST agar bodi plastik konektor tidak saling menjepit/bertabrakan saat kabel dicabut-pasang.
* **Posisi Port Micro-USB:** Menghadap ke **SISI KIRI** PCB Matrix.
* **Sisi Atas (Top Row ESP32):** Mulai dari pin **VIN** (kiri dekat USB) hingga pin **EN** (kanan dekat antena) pada **Baris 10**.
* **Sisi Bawah (Bottom Row ESP32):** Mulai dari pin **3.3V** (kiri dekat USB) hingga pin **D23** (kanan dekat antena) pada **Baris 20**.

Fungsi pin ESP32 pada kedua baris (sesuai label di diagram SVG):

```text
Baris 10:          VIN GND D13[TL] D12[L2] D14[R] D27[R] D26[L1] D25[A2] D33[L3] D32[L4] D35 D34 VN  VP  EN
                   │
[PORT USB DI KIRI] ├── [MODUL ESP32 DEVKIT V1 - 30 PINS] ── [ANTENA KUNING DI KANAN]
                   │
Baris 20:          3V3 GND D15[M] D2[HS] D4[A1] RX2[B1] TX2[B2] D5[TL] D18[D1] D19[D2] D21[C1] D22[C2] TX0 RX0 D23[TL]
```

Daftar soket JST per sisi (urutan huruf soket, bukan urutan posisi kiri-kanan):

* **Sisi Atas:** `[A]` J_IR1 [3P], `[B]` J_IR2 [3P], `[C]` J_LPJ_ATAS [4P], `[D]` LPJ_BWH 3&4 [3P], `[E]` SOLAR [2P], `[F]` PWR_IN [2P], `[G]` MOTOR [2P], `[H]` LPJ_BWH 1 [2P], `[I]` LDR [2P].
* **Sisi Bawah:** `[J]` MOTOR_DRV [2P], `[K]` BNC_EXT [3P], `[L]` HOUSE 1-4 [2P], `[M]` LPJ_BWH 2 [2P], `[N]` BNC_BC1 [3P], `[O]` BNC_A_SRC [4P], `[P]` J_TL VKS [4P].

Posisi grid yang diketahui dari diagram SVG: `[A]` di Kolom C, D, E; `[B]` di Kolom G, H, I; `[E]` di Kolom V, W; soket baru `[K]` di Baris 24-25 (Kolom B). Soket lain memakai ulang posisi soket lama yang fungsinya digantikan (ditandai "Eks ..." di diagram). Detail tiap soket ada di Bagian 4.

> [!NOTE]
> **Status Perakitan Fisik Terpasang:**
> 1. **Pojok Kiri Atas PCB:** `[A]` J_IR1 [JST 3-Pin] terpasang di Kolom **C, D, E (Baris 1–2)** tepat di samping lubang baut kiri.
> 2. **Sisi Kanan Atas PCB:** `[E]` SOLAR [JST 2-Pin] terpasang di Kolom **V, W (Baris 1–2)** tepat di samping lubang baut kanan.
> 3. **Satu-satunya header JST baru di revisi ini:** `[K]` BNC_EXT [JST 3-Pin] di Baris 24-25. Sisanya memakai ulang header yang sudah terpasang.

---

## 2. Pengelompokan SISI ATAS ESP32 (VIN s/d EN)

Pin pada sisi atas mikrokontroler membentang dari kiri (dekat USB) ke kanan (dekat antena):

| No | Nama Pin ESP32 | Fungsi / Modul Diorama | Soket JST Terkait | Lokasi Header JST yang Disarankan | Komponen Pasif di PCB Matrix |
| :-: | :--- | :--- | :---: | :--- | :--- |
| 1 | **VIN** | Input Daya +5V Utama & Jalur Dinamo | `J_PWR_IN` / Soket `[F]` | Kiri Atas (dekat port USB) | Titik temu rel daya +5V bersama |
| 2 | **GND** | Ground Utama Sisi Atas | Rel Ground | Sepanjang rel atas PCB | Bus rel tembaga GND |
| 3 | **D13** | Traffic Light Hijau | Bagian dari `J_TL` / Soket `[P]` | Kiri Atas / Tengah | Resistor 100 Ohm ke anoda LED hijau |
| 4 | **D12** | LPJ Bawah 2 (Lampu Simulator 2) | Soket `[M]` | Sisi Bawah (dipindah dari HOUSE 3-4) | Resistor 100 Ohm ke Pin 1 JST |
| 5 | **D14** | LPJ Jalur Atas 1 (Terpicu IR 1, malam hari) | `J_LPJ_ATAS` / Soket `[C]` (Pin 1) | Sisi Atas Tengah | Resistor 100 Ohm ke Pin 1 JST |
| 6 | **D27** | LPJ Jalur Atas 2 (Terpicu IR 2, malam hari) | `J_LPJ_ATAS` / Soket `[C]` (Pin 3) | Sisi Atas Tengah | Resistor 100 Ohm ke Pin 3 JST |
| 7 | **D26** | LPJ Bawah 1 (Lampu Simulator 1) | Soket `[H]` (Pin 1) | Sisi Atas Tengah | Resistor 100 Ohm ke Pin 1 JST |
| 8 | **D25** | Port BNC A2 (Kutub 2 Lampu 1) | Soket `[O]` (Pin 3) | Sisi Bawah / Menuju Panel BNC | Resistor seri 150 Ohm di belakang soket |
| 9 | **D33** | LPJ Bawah 3 (Lampu Simulator 3) | Soket `[D]` (Pin 3) | Sisi Atas Tengah | Resistor 100 Ohm ke Pin 3 JST |
| 10 | **D32** | LPJ Bawah 4 (Lampu Simulator 4) | Soket `[D]` (Pin 1) | Sisi Atas Tengah | Resistor 100 Ohm ke Pin 1 JST |
| 11 | **D35** | Sensor IR Proximity 2 | `J_IR2` / Soket `[B]` (JST 3-Pin) | Sisi Kiri Atas | Langsung dari OUT sensor (Input-Only) |
| 12 | **D34** | Sensor IR Proximity 1 | `J_IR1` / Soket `[A]` (JST 3-Pin) | **★ POJOK KIRI ATAS (Terpasang)** | Langsung dari OUT sensor (Input-Only) |
| 13 | **VN (GPIO 39)**| Input Sinyal Optocoupler PC817 | `J_SOLAR` / Soket `[E]` (JST 2-Pin)| **★ KANAN ATAS (Terpasang)** | Pin 4 PC817 + Resistor 10k Ohm ke 3.3V |
| 14 | **VP (GPIO 36)**| Modul LDR Digital DO (Siang/Malam) | Soket `[I]` (JST 2-Pin) | Sisi Kanan Atas (dekat VN) | DO modul ke VP. VCC modul lewat saklar toggle seri |
| 15 | **EN** | *Reset Chip (NC)* | - | Kanan Atas | Dibiarkan kosong |

---

## 3. Pengelompokan SISI BAWAH ESP32 (3.3V s/d D23)

Pin pada sisi bawah mikrokontroler membentang dari kiri (dekat USB) ke kanan (dekat antena):

| No | Nama Pin ESP32 | Fungsi / Modul Diorama | Soket JST Terkait | Lokasi Header JST yang Disarankan | Komponen Pasif di PCB Matrix |
| :-: | :--- | :--- | :---: | :--- | :--- |
| 1 | **3V3** | Output Tegangan 3.3V ESP32 | Rel Daya 3.3V | Kiri Bawah (dekat USB) | Mensuplai Sensor IR, LDR (lewat saklar toggle), & Pull-up 10k |
| 2 | **GND** | Ground Utama Sisi Bawah | Rel Ground | Sepanjang rel bawah PCB | Bus rel tembaga GND |
| 3 | **D15** | Driver Motor L298N IN1+IN3 | Soket `[J]` (JST 2-Pin) | Kiri Bawah | Output PWM ke modul L298N (eks saklar toggle sensor) |
| 4 | **D2** | 4 LED Rumah (Rumah 1-4) | Soket `[L]` (JST 2-Pin) | Sisi Bawah Tengah | 4x Resistor 100 Ohm paralel ke Pin 1 JST |
| 5 | **D4** | Port BNC A1 (Kutub 1 Lampu 1) | Soket `[O]` (Pin 1) | Sisi Bawah (Menuju BNC) | Resistor seri 150 Ohm di belakang soket |
| 6 | **RX2 (GPIO 16)**| Port BNC B1 (Kutub 1 Lampu 2) | Soket `[N]` (Pin 1) | Sisi Bawah (Menuju BNC) | Resistor seri 150 Ohm di belakang soket |
| 7 | **TX2 (GPIO 17)**| Port BNC B2 (Kutub 2 Lampu 2) | Soket `[N]` (Pin 3) | Sisi Bawah (Menuju BNC) | Resistor seri 150 Ohm di belakang soket |
| 8 | **D5** | Traffic Light Kuning | Bagian dari `J_TL` / Soket `[P]` | Sisi Bawah Kanan | Resistor 100 Ohm ke anoda LED kuning |
| 9 | **D18** | Port BNC D1 (Kutub 1 Lampu 4) | Soket `[K]` (Pin 1) | Sisi Bawah (soket baru B24-25) | Resistor seri 150 Ohm di belakang soket |
| 10 | **D19** | Port BNC D2 (Kutub 2 Lampu 4) | Soket `[K]` (Pin 2) | Sisi Bawah (soket baru B24-25) | Resistor seri 150 Ohm di belakang soket |
| 11 | **D21** | Port BNC C1 (Kutub 1 Lampu 3) | Soket `[N]` (Pin 2) | Sisi Bawah (Menuju BNC) | Resistor seri 150 Ohm di belakang soket |
| 12 | **D22** | Port BNC C2 (Kutub 2 Lampu 3) | Soket `[K]` (Pin 3) | Sisi Bawah (soket baru B24-25) | Resistor seri 150 Ohm di belakang soket |
| 13 | **TX0** | *Serial TX Debug (NC)* | - | - | Dibiarkan kosong |
| 14 | **RX0** | *Serial RX Debug (NC)* | - | - | Dibiarkan kosong |
| 15 | **D23** | Traffic Light Merah | Bagian dari `J_TL` / Soket `[P]` | Sisi Bawah Kanan | Resistor 100 Ohm ke anoda LED merah |

---

## 4. Daftar Rinci Pinout Seluruh Soket JST-XH (Molex)

### A. Soket Sisi Atas PCB Matrix

1. **`[A]` J_IR1, Sensor IR Proximity 1 (JST 3-Pin) ★ [Pojok Kiri Atas Terpasang]**
   * Pin 1 : VCC (3.3V)
   * Pin 2 : GND
   * Pin 3 : OUT (ke GPIO 34)
2. **`[B]` J_IR2, Sensor IR Proximity 2 (JST 3-Pin) [Kiri Atas]**
   * Pin 1 : VCC (3.3V)
   * Pin 2 : GND
   * Pin 3 : OUT (ke GPIO 35)
3. **`[F]` J_PWR_IN, Input Daya 5V Utama (JST 2-Pin) [Kiri Atas dekat USB]**
   * Pin 1 : +5V DC (dari modul Step-Down MP1584EN)
   * Pin 2 : GND (Ground bersama)
4. **`[G]` MOTOR, Catu Daya Motor (JST 2-Pin) [Kiri Atas]**
   * Pin 1 : +5V Switched (lewat saklar fisik kincir, ke catu daya modul L298N)
   * Pin 2 : GND
5. **`[C]` J_LPJ_ATAS, LPJ Jalur Atas (JST 4-Pin) [Tengah Atas]**
   * Pin 1 : Anoda LED LPJ Atas 1 (dari GPIO 14 via R 100 Ohm)
   * Pin 2 : GND
   * Pin 3 : Anoda LED LPJ Atas 2 (dari GPIO 27 via R 100 Ohm)
   * Pin 4 : GND
6. **`[H]` LPJ_BWH 1, LPJ Jalur Bawah 1 (JST 2-Pin) [Tengah Atas]**
   * Pin 1 : Anoda LED LPJ Bawah 1 (dari GPIO 26 via R 100 Ohm di PCB)
   * Pin 2 : GND
7. **`[D]` LPJ_BWH 3&4, LPJ Jalur Bawah 3 & 4 (JST 3-Pin) [Tengah Atas]**
   * Pin 1 : Anoda LED LPJ Bawah 4 (dari GPIO 32 via R 100 Ohm di PCB)
   * Pin 2 : **Common GND** (Katoda kedua LED digabung ke jalur ini)
   * Pin 3 : Anoda LED LPJ Bawah 3 (dari GPIO 33 via R 100 Ohm di PCB)
8. **`[I]` LDR, Modul Sensor Cahaya LDR (JST 2-Pin) [Kanan Atas dekat VN/VP]**
   * Pin 1 : Jalur 3.3V ESP32 (lewat saklar toggle seri)
   * Pin 2 : DO modul LDR (ke GPIO 36 / VP). DO HIGH berarti malam, DO LOW berarti siang
9. **`[E]` SOLAR, Input Panel Surya (JST 2-Pin) ★ [Kanan Atas Terpasang]**
   * Pin 1 : Anode A (+) Surya *(di PCB masuk ke Resistor 220 Ohm lalu ke Pin 1 PC817)*
   * Pin 2 : Cathode C (-) Surya *(di PCB terhubung langsung ke Pin 2 PC817)*

---

### B. Soket Sisi Bawah PCB Matrix

1. **`[J]` MOTOR_DRV, Kendali Driver L298N (JST 2-Pin) [Kiri Bawah]**
   * Pin 1 : Sinyal kendali GPIO 15 ESP32 (PWM, ke IN1+IN3 L298N)
   * Pin 2 : GND ESP32
2. **`[K]` BNC_EXT, Port BNC D1, D2, C2 (JST 3-Pin) [Baru, Baris 24-25]**
   * Pin 1 : Sinyal port D1 (ke GPIO 18, resistor seri 150 Ohm)
   * Pin 2 : Sinyal port D2 (ke GPIO 19, resistor seri 150 Ohm)
   * Pin 3 : Sinyal port C2 (ke GPIO 22, resistor seri 150 Ohm)
3. **`[L]` HOUSE 1-4, Penerangan Rumah 1-4 (JST 2-Pin) [Tengah Bawah]**
   * Pin 1 : Anoda 4 LED Rumah (dari GPIO 2 via 4x R 100 Ohm paralel di PCB)
   * Pin 2 : GND
4. **`[M]` LPJ_BWH 2, LPJ Jalur Bawah 2 (JST 2-Pin) [Tengah Bawah]**
   * Pin 1 : Anoda LED LPJ Bawah 2 (dari GPIO 12 via R 100 Ohm di PCB, eks HOUSE 3-4)
   * Pin 2 : GND
5. **`[O]` BNC_A_SRC, Port BNC A1, SRC+, A2, SRC- (JST 4-Pin) [Tengah/Kanan Bawah]**
   * Pin 1 : Sinyal port A1 (ke GPIO 4, resistor seri 150 Ohm)
   * Pin 2 : SRC+ 3.3V (lewat resistor 1 kOhm)
   * Pin 3 : Sinyal port A2 (ke GPIO 25, resistor seri 150 Ohm)
   * Pin 4 : SRC- (GND)
6. **`[N]` BNC_BC1, Port BNC B1, C1, B2 (JST 3-Pin) [Tengah/Kanan Bawah]**
   * Pin 1 : Sinyal port B1 (ke GPIO 16 / RX2, resistor seri 150 Ohm)
   * Pin 2 : Sinyal port C1 (ke GPIO 21, resistor seri 150 Ohm)
   * Pin 3 : Sinyal port B2 (ke GPIO 17 / TX2, resistor seri 150 Ohm)
7. **`[P]` J_TL, Modul Traffic Light VKS (JST 4-Pin) [Pojok Kanan Bawah]**
   * Pin 1 : Merah / R (dari GPIO 23 via R 100 Ohm di PCB)
   * Pin 2 : Kuning / Y (dari GPIO 5 via R 100 Ohm di PCB)
   * Pin 3 : Hijau / G (dari GPIO 13 via R 100 Ohm di PCB)
   * Pin 4 : GND Modul VKS

---

## 5. Rekapitulasi Fisik Header JST di PCB Matrix

* **Total JST 2-Pin : 8 Unit**
  * Sisi Atas: `[E]` SOLAR, `[F]` PWR_IN, `[G]` MOTOR, `[H]` LPJ_BWH 1, `[I]` LDR (5 unit)
  * Sisi Bawah: `[J]` MOTOR_DRV, `[L]` HOUSE 1-4, `[M]` LPJ_BWH 2 (3 unit)
* **Total JST 3-Pin : 5 Unit**
  * Sisi Atas: `[A]` J_IR1, `[B]` J_IR2, `[D]` LPJ_BWH 3&4 (3 unit)
  * Sisi Bawah: `[K]` BNC_EXT (baru), `[N]` BNC_BC1 (2 unit)
* **Total JST 4-Pin : 3 Unit**
  * Sisi Atas: `[C]` J_LPJ_ATAS (1 unit)
  * Sisi Bawah: `[O]` BNC_A_SRC, `[P]` J_TL (2 unit)
* **Grand Total: 16 Header JST-XH Male di PCB Matrix** (bertambah 1 header 3-pin baru, `[K]` BNC_EXT, dibanding revisi lama yang 15).
