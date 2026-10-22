# Panduan Wiring PCB Matrix & Pemetaan Konektor JST (Molex)

Dokumen ini memuat panduan tata letak (*layout*) dan penyolderan pada **PCB Matrix (Perfboard)** dengan orientasi mikrokontroler **ESP32 Dev Module (30-pin)**, dikelompokkan secara fisik antara **Sisi Atas** dan **Sisi Bawah**, serta pemetaan soket **Molex JST-XH 2.54mm (2-Pin, 3-Pin, 4-Pin)**.

---

> [!TIP]
> **Diagram Visual Vektor Grid (A-Y × 1-29):**
> Anda dapat melihat atau membuka gambar visual denah tata letak soket JST lengkap pada berkas:
> 👉 **[docs/pcb_matrix_layout.svg](file:///Volumes/Data%20Shared/Project/SmartCity/docs/pcb_matrix_layout.svg)**

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

```text
       [BAUT 2x2]                                                            [BAUT 2x2]
       (A-B, 1-2)    C-E   F   G-I   J   K-N   O   P-R      S-U      V-W    (X-Y, 1-2)
      ┌───────────┐ ┌───┐ ┌─┐ ┌───┐ ┌─┐ ┌───┐ ┌─┐ ┌───┐ ┌─────────┐ ┌───┐ ┌───────────┐
Baris │ [LUBANG   │ │IR1│ │ │ │IR2│ │ │ │LPJ│ │ │ │PAR│ │ AREA R   │ │SOL│ │ [LUBANG   │
1 - 2 │   BAUT]   │ │★  │ │S│ │   │ │S│ │ATS│ │S│ │   │ │ OPTO 10k │ │★  │ │   BAUT]   │
      └───────────┘ └───┘ └─┘ └───┘ └─┘ └───┘ └─┘ └───┘ └─────────┘ └───┘ └───────────┘
                    (3P)      (3P)      (4P)      (3P)                 (2P)
...
Baris 10:          VIN GND D13 D12 D14 D27 D26 D25 D33 D32 D35 D34 VN  VP  EN
                   │
[PORT USB DI KIRI] ├── [MODUL ESP32 DEVKIT V1 - 30 PINS] ── [ANTENA KUNING DI KANAN]
                   │
Baris 20:          3V3 GND D15 D2  D4  RX2 TX2 D5  D18 D19 D21 D22 TX0 RX0 D23
...
      ┌───────────┐ ┌───┐ ┌─┐ ┌───┐ ┌─┐ ┌───┐ ┌─┐ ┌───┐ ┌─┐ ┌───┐     ┌─┐ ┌───────────┐
Baris │ [LUBANG   │ │HS1│ │ │ │HS2│ │ │ │BNC│ │ │ │SER│ │ │ │TL │     │ │ │ [LUBANG   │
28-29 │   BAUT]   │ │   │ │S│ │   │ │S│ │PAR│ │S│ │   │ │S│ │   │     │ │ │   BAUT]   │
      └───────────┘ └───┘ └─┘ └───┘ └─┘ └───┘ └─┘ └───┘ └─┘ └───┘     └─┘ └───────────┘
       (A-B, 28-29)  C-D   E   F-G   H   I-K   L   M-P   Q   R-U   V-W  (X-Y, 28-29)
                    (2P)      (2P)      (3P)      (4P)      (4P)

★ Catatan: J_SW_SENS (2-Pin) diletakkan di Baris 24-25 (Kolom E-F) tepat di bawah pin D15 ESP32.
```

> [!NOTE]
> **Status Perakitan Fisik Terpasang:**
> 1. **Pojok Kiri Atas PCB:** `J_IR1` [JST 3-Pin] terpasang di Kolom **C, D, E (Baris 1–2)** tepat di samping lubang baut kiri.
> 2. **Sisi Kanan Atas PCB:** `J_SOLAR` [JST 2-Pin] terpasang di Kolom **V, W (Baris 1–2)** tepat di samping lubang baut kanan.


---

## 2. Pengelompokan SISI ATAS ESP32 (VIN s/d EN)

Pin pada sisi atas mikrokontroler membentang dari kiri (dekat USB) ke kanan (dekat antena):

| No | Nama Pin ESP32 | Fungsi / Modul Diorama | Soket JST Terkait | Lokasi Header JST yang Disarankan | Komponen Pasif di PCB Matrix |
| :-: | :--- | :--- | :---: | :--- | :--- |
| 1 | **VIN** | Input Daya +5V Utama & Jalur Dinamo | `J_PWR_IN` / `J_MOTOR` | Kiri Atas (dekat port USB) | Titik temu rel daya +5V bersama |
| 2 | **GND** | Ground Utama Sisi Atas | Rel Ground | Sepanjang rel atas PCB | Bus rel tembaga GND |
| 3 | **D13** | Traffic Light Hijau | Bagian dari `J_TL` | Kiri Atas / Tengah | Resistor 100 Ohm ke anoda LED hijau |
| 4 | **D12** | *Tidak Digunakan (NC)* | - | - | Dibiarkan kosong |
| 5 | **D14** | LPJ Jalur Atas 1 (Terpicu IR 1) | `J_LPJ_ATAS` (Pin 1) | Sisi Atas Tengah | Resistor 100 Ohm ke Pin 1 JST |
| 6 | **D27** | LPJ Jalur Atas 2 (Terpicu IR 2) | `J_LPJ_ATAS` (Pin 3) | Sisi Atas Tengah | Resistor 100 Ohm ke Pin 3 JST |
| 7 | **D26** | LPJ Bawah Seri (Anti-Curang) | `J_LPJ_SERI` (Pin 1) | Sisi Atas Tengah | 2x Resistor 100 Ohm paralel ke Pin 1 |
| 8 | **D25** | BNC Seri Pasangan 2 (Anti-Curang) | `J_BNC_SERI` (Pin 3) | Sisi Atas / Menuju Panel BNC | Langsung ke Pin 3 JST (Internal Pull-Up) |
| 9 | **D33** | LPJ Bawah Paralel 2 | `J_LPJ_BAWAH` (Pin 3)| Sisi Atas Tengah | Resistor 100 Ohm ke Pin 3 JST |
| 10 | **D32** | LPJ Bawah Paralel 1 | `J_LPJ_BAWAH` (Pin 1)| Sisi Atas Tengah | Resistor 100 Ohm ke Pin 1 JST |
| 11 | **D35** | Sensor IR Proximity 2 | `J_IR2` (JST 3-Pin) | Sisi Kiri Atas | Langsung dari OUT sensor (Input-Only) |
| 12 | **D34** | Sensor IR Proximity 1 | `J_IR1` (JST 3-Pin) | **★ POJOK KIRI ATAS (Terpasang)**| Langsung dari OUT sensor (Input-Only) |
| 13 | **VN (GPIO 39)**| Input Sinyal Optocoupler PC817 | `J_SOLAR` (JST 2-Pin)| **★ KANAN ATAS (Terpasang)** | Pin 4 PC817 + Resistor 10k Ohm ke 3.3V |
| 14 | **VP (GPIO 36)**| Sensor Cahaya LDR (Siang/Malam) | `J_LDR` (JST 2-Pin) | Sisi Kanan Atas (dekat VN) | Titik tengah LDR + Resistor 10k Ohm ke GND |
| 15 | **EN** | *Reset Chip (NC)* | - | Kanan Atas | Dibiarkan kosong |

---

## 3. Pengelompokan SISI BAWAH ESP32 (3.3V s/d D23)

Pin pada sisi bawah mikrokontroler membentang dari kiri (dekat USB) ke kanan (dekat antena):

| No | Nama Pin ESP32 | Fungsi / Modul Diorama | Soket JST Terkait | Lokasi Header JST yang Disarankan | Komponen Pasif di PCB Matrix |
| :-: | :--- | :--- | :---: | :--- | :--- |
| 1 | **3V3** | Output Tegangan 3.3V ESP32 | Rel Daya 3.3V | Kiri Bawah (dekat USB) | Mensuplai Sensor IR, LDR, & Pull-up 10k |
| 2 | **GND** | Ground Utama Sisi Bawah | Rel Ground | Sepanjang rel bawah PCB | Bus rel tembaga GND |
| 3 | **D15** | Saklar Toggle Sensor (ON/OFF) | `J_SW_SENS` (JST 2-Pin)| Kiri Bawah | Langsung ke saklar (Internal Pull-Up) |
| 4 | **D2** | *Bebas (NC / Onboard LED)* | - | - | Dibiarkan kosong |
| 5 | **D4** | BNC Seri Pasangan 1 | `J_BNC_SERI` (Pin 1) | Sisi Bawah (Menuju BNC) | Langsung ke Pin 1 JST (Internal Pull-Up) |
| 6 | **RX2 (GPIO 16)**| BNC Paralel Cabang 1 | `J_BNC_PAR` (Pin 1) | Sisi Bawah (Menuju BNC) | Langsung ke Pin 1 JST (Internal Pull-Up) |
| 7 | **TX2 (GPIO 17)**| BNC Paralel Cabang 2 | `J_BNC_PAR` (Pin 3) | Sisi Bawah (Menuju BNC) | Langsung ke Pin 3 JST (Internal Pull-Up) |
| 8 | **D5** | Traffic Light Kuning | Bagian dari `J_TL` | Sisi Bawah Kanan | Resistor 100 Ohm ke anoda LED kuning |
| 9 | **D18** | Rumah 1 & Rumah 2 (2 LED) | `J_HOUSE_1_2` (2-Pin) | Sisi Bawah Tengah | 2x Resistor 100 Ohm paralel ke Pin 1 JST |
| 10 | **D19** | Rumah 3 & Rumah 4 (2 LED) | `J_HOUSE_3_4` (2-Pin) | Sisi Bawah Tengah | 2x Resistor 100 Ohm paralel ke Pin 1 JST |
| 11 | **D21** | *Bebas / Reserved (NC)* | - | - | Dibiarkan kosong |
| 12 | **D22** | *Bebas / Reserved (NC)* | - | - | Dibiarkan kosong |
| 13 | **TX0** | *Serial TX Debug (NC)* | - | - | Dibiarkan kosong |
| 14 | **RX0** | *Serial RX Debug (NC)* | - | - | Dibiarkan kosong |
| 15 | **D23** | Traffic Light Merah | Bagian dari `J_TL` | Sisi Bawah Kanan | Resistor 100 Ohm ke anoda LED merah |

---

## 4. Daftar Rinci Pinout Seluruh Soket JST-XH (Molex)

### A. Soket Sisi Atas PCB Matrix

1. **`J_IR1` — Sensor IR Proximity 1 (JST 3-Pin) ★ [Pojok Kiri Atas Terpasang]**
   * Pin 1 : VCC (3.3V)
   * Pin 2 : GND
   * Pin 3 : OUT (ke GPIO 34)
2. **`J_IR2` — Sensor IR Proximity 2 (JST 3-Pin) [Kiri Atas]**
   * Pin 1 : VCC (3.3V)
   * Pin 2 : GND
   * Pin 3 : OUT (ke GPIO 35)
3. **`J_PWR_IN` — Input Daya 5V Utama (JST 2-Pin) [Kiri Atas dekat USB]**
   * Pin 1 : +5V DC (dari modul Step-Down MP1584EN)
   * Pin 2 : GND (Ground bersama)
4. **`J_MOTOR` — Dinamo Kincir Angin (JST 2-Pin) [Kiri Atas]**
   * Pin 1 : +5V Switched (lewat saklar fisik kincir)
   * Pin 2 : GND *(Di PCB dipasang Dioda 1N4007 paralel terbalik & Elco 470uF)*
5. **`J_LPJ_ATAS` — LPJ Jalur Atas (JST 4-Pin) [Tengah Atas]**
   * Pin 1 : Anoda LED LPJ Atas 1 (dari GPIO 14 via R 100 Ohm)
   * Pin 2 : GND
   * Pin 3 : Anoda LED LPJ Atas 2 (dari GPIO 27 via R 100 Ohm)
   * Pin 4 : GND
6. **`J_LPJ_SERI` — LPJ Jalur Bawah Seri (JST 2-Pin) [Tengah Atas]**
   * Pin 1 : Anoda 2 LED Seri (dari GPIO 26 via 2x R 100 Ohm paralel di PCB)
   * Pin 2 : GND
7. **`J_LPJ_BAWAH` — LPJ Jalur Bawah Paralel (JST 3-Pin) [Tengah Atas]**
   * Pin 1 : Anoda LED Paralel 1 (dari GPIO 32 via R 100 Ohm di PCB)
   * Pin 2 : **Common GND** (Katoda kedua LED digabung ke jalur ini)
   * Pin 3 : Anoda LED Paralel 2 (dari GPIO 33 via R 100 Ohm di PCB)
8. **`J_LDR` — Sensor Cahaya LDR (JST 2-Pin) [Kanan Atas dekat VN/VP]**
   * Pin 1 : Jalur 3.3V ESP32
   * Pin 2 : Jalur Masukan Sensor *(di PCB terhubung ke GPIO 36 dan Resistor 10k ke GND)*
9. **`J_SOLAR` — Input Panel Surya (JST 2-Pin) ★ [Kanan Atas Terpasang]**
   * Pin 1 : Anode A (+) Surya *(di PCB masuk ke Resistor 220 Ohm lalu ke Pin 1 PC817)*
   * Pin 2 : Cathode C (-) Surya *(di PCB terhubung langsung ke Pin 2 PC817)*

---

### B. Soket Sisi Bawah PCB Matrix

1. **`J_SW_SENS` — Saklar Toggle Sensor (JST 2-Pin) [Kiri Bawah]**
   * Pin 1 : Jalur GPIO 15 ESP32 (`INPUT_PULLUP`)
   * Pin 2 : GND ESP32
2. **`J_HOUSE_1_2` — Penerangan Rumah 1 & 2 (JST 2-Pin) [Tengah Bawah]**
   * Pin 1 : Anoda Rumah 1 & 2 (dari GPIO 18 via 2x R 100 Ohm paralel di PCB)
   * Pin 2 : GND
3. **`J_HOUSE_3_4` — Penerangan Rumah 3 & 4 (JST 2-Pin) [Tengah Bawah]**
   * Pin 1 : Anoda Rumah 3 & 4 (dari GPIO 19 via 2x R 100 Ohm paralel di PCB)
   * Pin 2 : GND
4. **`J_BNC_SERI` — Simulator BNC Seri Anti-Curang (JST 4-Pin) [Tengah/Kanan Bawah]**
   * Pin 1 : Sinyal BNC Pasangan 1 (ke GPIO 4)
   * Pin 2 : GND (ke Lubang BNC Pasangan 1)
   * Pin 3 : Sinyal BNC Pasangan 2 (ke GPIO 25 - Anti-Curang)
   * Pin 4 : GND (ke Lubang BNC Pasangan 2)
5. **`J_BNC_PAR` — Simulator BNC Paralel (JST 3-Pin) [Tengah/Kanan Bawah]**
   * Pin 1 : Sinyal BNC Cabang 1 (ke GPIO 16 / RX2)
   * Pin 2 : **Common GND** (Ground bersama untuk kedua lubang BNC Paralel)
   * Pin 3 : Sinyal BNC Cabang 2 (ke GPIO 17 / TX2)
6. **`J_TL` — Modul Traffic Light VKS (JST 4-Pin) [Pojok Kanan Bawah]**
   * Pin 1 : Merah / R (dari GPIO 23 via R 100 Ohm di PCB)
   * Pin 2 : Kuning / Y (dari GPIO 5 via R 100 Ohm di PCB)
   * Pin 3 : Hijau / G (dari GPIO 13 via R 100 Ohm di PCB)
   * Pin 4 : GND Modul VKS

---

## 5. Rekapitulasi Fisik Header JST di PCB Matrix

* **Total JST 2-Pin : 8 Unit**
  * Sisi Atas: `J_PWR_IN`, `J_MOTOR`, `J_LPJ_SERI`, `J_LDR`, `J_SOLAR` (5 unit)
  * Sisi Bawah: `J_SW_SENS`, `J_HOUSE_1_2`, `J_HOUSE_3_4` (3 unit)
* **Total JST 3-Pin : 4 Unit**
  * Sisi Atas: `J_IR1`, `J_IR2`, `J_LPJ_BAWAH` (3 unit)
  * Sisi Bawah: `J_BNC_PAR` (1 unit)
* **Total JST 4-Pin : 3 Unit**
  * Sisi Atas: `J_LPJ_ATAS` (1 unit)
  * Sisi Bawah: `J_BNC_SERI`, `J_TL` (2 unit)
* **Grand Total: 15 Header JST-XH Male di PCB Matrix.**

