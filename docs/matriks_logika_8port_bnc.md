# Matriks Logika & Spesifikasi Teknis Simulator 10-Port BNC (Sistem Sumber + Beban Adaptif)

Dokumen ini memuat spesifikasi teknis lengkap, analisis rangkaian, pemetaan pin GPIO bebas *strapping*, algoritma *dual-read* (Pull-up/Pull-down), analisis nodal, serta proteksi perangkat keras untuk **Simulator Kelistrikan 10-Port BNC** berbasis mikrokontroler ESP32.

---

## 1. Topologi Panel & Perangkat Keras 10 Port BNC

Sistem menyediakan 2 terminal Sumber Daya dan 8 terminal Beban Lampu:

```text
┌────────────────────────────────────────────────────────────────────────┐
│                        PANEL 10 PORT BNC                               │
│                                                                        │
│   [ SOURCE (+) ]                             [ SOURCE (-) ]            │
│   (Terminal Positif 3.3V via 1 kΩ)           (Terminal Negatif / GND)  │
│                                                                        │
│   [ A1 ] ─── ( LAMPU 1 ) ─── [ A2 ]          (LPJ Kiri Bawah 1)        │
│   [ B1 ] ─── ( LAMPU 2 ) ─── [ B2 ]          (LPJ Kiri Bawah 2)        │
│   [ C1 ] ─── ( LAMPU 3 ) ─── [ C2 ]          (LPJ Kanan Bawah 1)       │
│   [ D1 ] ─── ( LAMPU 4 ) ─── [ D2 ]          (LPJ Kanan Bawah 2)       │
└────────────────────────────────────────────────────────────────────────┘
```

### Catatan Fisik Konektor BNC vs Jack Banana:
* Kabel BNC adalah kabel koaksial (memiliki pin tengah dan *shield* luar). Pada sistem ini, sinyal dialirkan melalui **pin tengah**, sedangkan *shield* luar dihubungkan ke pelindung/chassis.
* **Percabangan Paralel:** Karena soket BNC panel tidak bisa ditumpuk langsung, siswa wajib disediakan **BNC T-Connector (BNC Male to 2x Female)** untuk membuat percabangan cabang paralel.
* *(Opsi Alternatif Masa Depan):* Jack Banana 4mm *stackable* (seperti kit praktikum fisika sekolah) adalah alternatif yang lebih ekonomis dan langsung dapat ditumpuk tanpa adapter T.

---

## 2. Pemetaan Pin GPIO ESP32 Bebas Masalah *Strapping Pins*

Untuk menjamin ESP32 tidak pernah gagal *boot* saat siswa menancapkan kabel sebelum alat dinyalakan, **seluruh port eksternal BNC hanya dialokasikan pada pin GPIO non-strapping**. Pin *strapping* sensitif (`GPIO 12`, `GPIO 2`, `GPIO 15`) diisolasi hanya untuk periferal internal yang statusnya terkontrol penuh saat *boot*.

### A. Alokasi 10 Port BNC & Pin Monitor Korslet (Mapping Awal 10-Port)
| No | Port Panel | Fungsi Rangkaian | Pin ESP32 | Soket JST di PCB Matrix | Sifat Pin & Proteksi Hardware |
| :-: | :---: | :--- | :---: | :--- | :--- |
| **1** | **`SRC (+)`** | Sumber Daya Positif (3.3V) | *Rel 3.3V* | Soket `[O]` Pin 2 | Lewat Resistor 1 kOhm (1/4W) sebagai pembatas arus |
| **2** | **`SRC (-)`** | Sumber Daya Negatif (GND) | *Rel GND* | Soket `[O]` Pin 4 | Ground bersama sistem |
| **3** | **`A1`** | Kutub 1 — Lampu 1 | **`GPIO 4`** | Soket `[O]` Pin 1 | Input/Output Dinamis + Resistor Seri 150 Ohm |
| **4** | **`A2`** | Kutub 2 — Lampu 1 | **`GPIO 25`** | Soket `[O]` Pin 3 | Input/Output Dinamis + Resistor Seri 150 Ohm |
| **5** | **`B1`** | Kutub 1 — Lampu 2 | **`GPIO 16`** | Soket `[N]` Pin 1 (RX2) | Input/Output Dinamis + Resistor Seri 150 Ohm |
| **6** | **`B2`** | Kutub 2 — Lampu 2 | **`GPIO 17`** | Soket `[N]` Pin 3 (TX2) | Input/Output Dinamis + Resistor Seri 150 Ohm |
| **7** | **`C1`** | Kutub 1 — Lampu 3 | **`GPIO 21`** | Soket `[N]` Pin 2 | Input/Output Dinamis + Resistor Seri 150 Ohm |
| **8** | **`C2`** | Kutub 2 — Lampu 3 | **`GPIO 22`** | Soket `[K]` Pin 3 | Input/Output Dinamis + Resistor Seri 150 Ohm |
| **9** | **`D1`** | Kutub 1 — Lampu 4 | **`GPIO 18`** | Soket `[K]` Pin 1 | Input/Output Dinamis + Resistor Seri 150 Ohm |
| **10**| **`D2`** | Kutub 2 — Lampu 4 | **`GPIO 19`** | Soket `[K]` Pin 2 | Input/Output Dinamis + Resistor Seri 150 Ohm |

### B. Alokasi Output 4 Lampu Jalan Miniatur (Mendukung PWM Kecerahan)
| Nama Beban | Tiang Diorama | Pin Output ESP32 | Fitur LED PWM (LEDC) | Soket PCB Matrix |
| :--- | :--- | :---: | :--- | :--- |
| **Lampu 1** | LPJ Bawah 1 | **`GPIO 26`** | Channel PWM 0 (Kecerahan adaptif) | `[H] LPJ_BWH 1 [2P]` |
| **Lampu 2** | LPJ Bawah 2 | **`GPIO 12`** | Channel PWM 1 (Kecerahan adaptif) | `[M] LPJ_BWH 2 [2P]` |
| **Lampu 3** | LPJ Bawah 3 | **`GPIO 33`** | Channel PWM 2 (Kecerahan adaptif) | `[D] LPJ_BWH 3&amp;4 [3P] (Pin 3)` |
| **Lampu 4** | LPJ Bawah 4 | **`GPIO 32`** | Channel PWM 3 (Kecerahan adaptif) | `[D] LPJ_BWH 3&amp;4 [3P] (Pin 1)` |

### C. Alokasi Periferal Lain (Internal Maket)
* **Motor Driver L298N (2x Dinamo DC 1-Arah):** **`GPIO 15`** pada Soket `[J]` (`1: D15 [PWM] | 2: GND`).
* **Lampu Rumah (4 LED):** Digabung ke **`GPIO 2`** pada Soket `[L]` (arus total ~12 mA via resistor 100 Ohm).
* **LPJ Jalur Atas (2 LED):** **`GPIO 14`** (LPJ Atas 1) dan **`GPIO 27`** (LPJ Atas 2) pada Soket `[C]`. *Hanya aktif bersiaga jika malam hari (`isNight == true`).*
* **Traffic Light VKS:** **`GPIO 23`** (Merah), **`GPIO 5`** (Kuning), **`GPIO 13`** (Hijau) pada Soket `[P]`.
* **Sensor Input-Only:** **`GPIO 34`** (IR 1 - Soket `[A]`), **`GPIO 35`** (IR 2 - Soket `[B]`), **`GPIO 36`** (LDR DO - Soket `[I]`), **`GPIO 39`** (Solar Optocoupler - Soket `[E]`).
* **Saklar Toggle Otomasi:** Dipasang seri fisik pada jalur kabel VCC (3.3V) ke Modul LDR (`[I]`). Saat OFF, LDR mati fisik sehingga terbaca Siang (`isNight = false`) dan seluruh lampu otomasi seketika padam aman.

---

## 3. Proteksi Listrik Port & Aturan Penggerak Pin (*Pin Drive Rule*)

Karena port BNC dapat dipegang langsung oleh siswa dan dihubungkan ke mana saja, proteksi listrik wajib diterapkan:

1. **Aturan Penggerak Open-Drain LOW Saja (Wajib):**
   * Saat melakukan pemindaian jembatan antar-port, pin ESP32 **HANYA BOLEH di-drive ke `OUTPUT LOW` (0V) atau dibiarkan sebagai `INPUT`**.
   * **DILARANG KERAS men-drive pin ke `OUTPUT HIGH` (3.3V)**. Jika sebuah pin di-drive `HIGH` lalu siswa mencolokkannya ke `SRC (-)` (GND), arus puluhan mA akan mengalir langsung ke chip dan merusak pin mikrokontroler.
2. **Resistor Pembatas Seri (150 Ohm):**
   * Di setiap jalur pin `A1 s/d D2` dipasang resistor seri **150 Ohm** (1/8W atau 1/4W) tepat di belakang soket BNC.
   * Nilai 150 Ohm cukup kecil sehingga tegangan `LOW` saat scan tetap terbaca valid (< 0.4V), namun cukup besar untuk menahan lonjakan arus jika terjadi kesalahan koneksi.
3. **Proteksi Sambungan Bebas Korslet:**
   * Terminal `SRC (+)` dilindungi resistor **1 kOhm**. Arus maksimal yang bisa ditarik jika terjadi korslet langsung ke `SRC (-)` hanyalah `3.3V / 1000 Ohm = 3.3 mA` (sangat dingin dan aman).

---

## 4. Deteksi Hubung Singkat / Korslet Sumber Daya (`SRC+` ke `SRC-`)

Terminal `SRC (+)` dilindungi resistor pembatas 1 kOhm, jadi arus maksimal saat korslet langsung ke `SRC (-)` hanyalah `3.3V / 1000 Ohm = 3.3 mA`.

### Logika Deteksi Korslet (Sesuai `src/main.cpp`, `scanBananaMatrix()`):
Deteksi tidak memakai pin sense terpisah. GPIO 23 sudah dipakai sebagai output Traffic Light Merah, jadi tidak bisa merangkap jadi pin sense. Sebagai gantinya, korslet dikenali dari hasil pemindaian graf:
1. **Kondisi Normal:** `SRC+` (Net VCC) dan `SRC-` (Net GND) berada di simpul yang berbeda.
2. **Kondisi Korslet Langsung (`SRC+` dicolok langsung ke `SRC-`):** pemindaian Union-Find menggabungkan kedua terminal ke satu net yang sama, `dsu.find(NODE_VCC) == dsu.find(NODE_GND)`, dan flag `isSrcShortCircuited` menjadi `true`.
3. **Respon Sistem:**
   * Keempat lampu jalan bawah langsung dipadamkan (`lpjBwhDuty[k] = 0`).
   * Sistem memberi peringatan di Web Dashboard / Serial Monitor:
     `"[WARNING] Hubung singkat terdeteksi antara SRC+ dan SRC-! Periksa perkabelan."`

---

## 5. Algoritma Pembacaan Ganda (Pull-Up & Pull-Down)

Dengan resistor *pull-up* internal saja, port yang mengambang (*floating*) dan port yang terhubung ke `SRC (+)` akan sama-sama terbaca `HIGH`. Untuk membedakannya secara mutlak, ESP32 membaca **dua kali berturut-turut** menggunakan mode *Pull-Up* dan *Pull-Down*:

```cpp
// Fungsi mengklasifikasikan status koneksi sebuah port
enum PortState { PORT_FLOATING, PORT_TO_VCC, PORT_TO_GND };

PortState readPortState(uint8_t pin) {
    // 1. Baca dengan Pull-Up Internal
    pinMode(pin, INPUT_PULLUP);
    delayMicroseconds(10); // Waktu settling kapasitansi kabel
    int valPU = digitalRead(pin);

    // 2. Baca dengan Pull-Down Internal
    pinMode(pin, INPUT_PULLDOWN);
    delayMicroseconds(10);
    int valPD = digitalRead(pin);

    // 3. Evaluasi Tabel Kebenaran
    if (valPU == HIGH && valPD == LOW) {
        return PORT_FLOATING; // Mengambang (tidak dicolok ke sumber)
    } else if (valPU == HIGH && valPD == HIGH) {
        return PORT_TO_VCC;    // Terhubung ke SRC (+) via kabel
    } else {
        return PORT_TO_GND;    // Terhubung ke SRC (-) / GND via kabel
    }
}
```

### Tabel Kebenaran Pembacaan:
| Status Fisik Kabel | Hasil Baca Pull-Up (`valPU`) | Hasil Baca Pull-Down (`valPD`) | Klasifikasi Node |
| :--- | :---: | :---: | :--- |
| **Tidak Dicolok (Floating)** | **`HIGH`** | **`LOW`** | Port Bebas / Mengambang |
| **Terhubung ke `SRC (+)`** | **`HIGH`** | **`HIGH`** | Node Positif Sumber (VCC) |
| **Terhubung ke `SRC (-)`** | **`LOW`** | **`LOW`** | Node Negatif Sumber (GND) |

---

## 6. Algoritma Analisis Nodal & Topologi Graf (Simetri & Beban Korslet)

Untuk memastikan sistem adaptif secara fisik:
1. **Lampu adalah Beban Simetris:** Membalik kabel antara kaki 1 dan kaki 2 (`A1 <-> A2`) tidak boleh menggagalkan rangkaian.
2. **Deteksi Lampu Terkorsleting (*Shorted Load*):** Jika kedua kaki lampu terhubung ke simpul/kabel yang sama, lampu tersebut harus **PADAM**, bukan menyala.

### Langkah Analisis Nodal (Graph Modeling):
1. **Langkah 1 — Segmentasi Simpul (*Net Identification*):**
   * Gunakan struktur data *Disjoint-Set (Union-Find)* untuk menggabungkan port-port yang saling terhubung kabel menjadi nomor Simpul (*Net ID*).
   * Tetapkan Simpul Khusus: `Net_VCC` (terhubung ke `SRC+`) dan `Net_GND` (terhubung ke `SRC-`).
2. **Langkah 2 — Evaluasi Status Masing-Masing Lampu:**
   Tiap Lampu `k` (memiliki Kaki 1 dan Kaki 2):
   * Tentukan simpul Kaki 1: `Net(Kaki 1)`.
   * Tentukan simpul Kaki 2: `Net(Kaki 2)`.
   * **Kasus Beban Terkorslet (Short Circuit):**  
     Jika `Net(Kaki 1) == Net(Kaki 2)`, maka beda potensial pada lampu adalah nol (V1 - V2 = 0).  
     **Status: Lampu `k` WAJIB PADAM (`OFF`).**
3. **Langkah 3 — Perhitungan Beda Potensial Rangkaian (Analisis Nodal Ringan):**
   * Karena jumlah simpul hanya 8–10 node, ESP32 memodelkan rangkaian sebagai graf resistansi sederhana (tiap lampu dianggap resistor setara R = 100 Ohm).
   * Hitung tegangan di tiap simpul terhadap `Net_GND`:
     * **1 Lampu Mandiri (atau Paralel Penuh):**  
       Tegangan pada lampu = 3.3 V. Lampu menyala **Terang Maksimal (PWM 100%)**.
     * **2 Lampu Seri:**  
       Tegangan terbagi rata: masing-masing lampu menerima ~1.65 V.  
       Lampu menyala **Redup (PWM 50%)**.
     * **3 Lampu Seri:**  
       Masing-masing lampu menerima ~1.10 V.  
       Lampu menyala **Sangat Redup (PWM 33%)**.
     * **4 Lampu Seri:**  
       Masing-masing lampu menerima ~0.82 V.  
       Lampu menyala **Redup Minimal (PWM 25%)**.
     * **Rangkaian Terbuka (Putus):**  
       Tidak ada jalur arus dari `Net_VCC` ke `Net_GND` melintasi lampu tersebut.  
       Tegangan jatuh = 0 V. **Lampu PADAM (`PWM 0%`)**.

> **Keunggulan Edukatif:** Fitur kecerahan berbasis PWM ini mendemonstrasikan secara visual hukum fisika nyata ke siswa SMP: *"Rangkaian seri membagi tegangan sehingga lampu meredup, sedangkan rangkaian paralel menerima tegangan penuh sehingga tetap terang!"*

---

## 7. Waktu Pemindaian (*Scan Timing*) & Filter Debouncing

* **Waktu Eksekusi Realistis:**  
  Dengan mempertimbangkan penundaan pengisian kapasitansi kabel (*settling time*) sebesar 10 µs per pembacaan dan pemindaian 8 port, satu siklus pemindaian lengkap memakan waktu sekitar **0.8 s/d 1.5 milidetik**.
* **Filter Debouncing Software (Peredam Flicker):**  
  * Hasil pembacaan graf tidak langsung mengubah status LED.
  * Status output lampu hanya diperbarui jika topologi rangkaian terbaca **stabil dan identik selama 2 s/d 3 siklus pemindaian berturut-turut** (~30–50 ms).
  * Hal ini menjamin lampu tidak berkedip (*flicker*) saat siswa sedang menancapkan atau mencabut konektor BNC.

---

## 8. Tabel Ringkasan Skenario Pengujian Sistem 10-Port

| No | Skenario Rangkaian | Pola Kabel Siswa | Status Nodal di ESP32 | Output Fisik Lampu Diorama |
| :-: | :--- | :--- | :--- | :--- |
| **1** | **1 Lampu Mandiri** | `SRC+` -> `A1`, `A2` -> `SRC-` | V_L1 = 3.3V | **Lampu 1 ON (100% Terang)** |
| **2** | **Kaki Lampu Dibalik** | `SRC+` -> `A2`, `A1` -> `SRC-` | V_L1 = 3.3V (Simetri) | **Lampu 1 ON (100% Terang)** |
| **3** | **Lampu 1 Terkorslet** | `SRC+` -> `A1`, kabel jumper `A1` ke `A2` | Net(A1) == Net(A2) (V = 0) | **Lampu 1 PADAM (Beban Korslet)** |
| **4** | **2 Lampu Seri Utuh** | `SRC+` -> `A1`, `A2` -> `B1`, `B2` -> `SRC-` | V_L1 = 1.65V, V_L2 = 1.65V | **Lampu 1 & 2 ON (Redup 50%)** |
| **5** | **Uji Seri: Cabut 1 Kabel** | Cabut kabel jembatan `A2` ke `B1` | Jalur VCC ke GND terputus | **Lampu 1 & 2 PADAM BERSAMAAN** |
| **6** | **2 Lampu Paralel Utuh** | `SRC+` cabang ke `A1` & `B1`; `SRC-` cabang ke `A2` & `B2` | V_L1 = 3.3V, V_L2 = 3.3V | **Lampu 1 & 2 ON (100% Terang)** |
| **7** | **Uji Paralel: Cabut 1 Cabang** | Cabut cabang `SRC+` ke `B1` | Lampu 1 utuh, Lampu 2 putus | **Lampu 1 TETAP ON, Lampu 2 PADAM** |
| **8** | **4 Lampu Seri Penuh** | `SRC+` -> `A` -> `B` -> `C` -> `D` -> `SRC-` | Tiap lampu V ~ 0.82V | **Ke-4 Lampu ON (Redup 25%)** |
| **9** | **Korslet Sumber Langsung** | `SRC (+)` dicolok langsung ke `SRC (-)` | Pin `GPIO 23` membaca `LOW` | **SEMUA PADAM + Peringatan Web** |
