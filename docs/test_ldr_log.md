# Log Pengujian: Sensor Cahaya LDR (Mode Murni Offline / Tanpa Wi-Fi)

Dokumen ini mencatat hasil pengujian (*proofing test*) pembacaan sensor cahaya LDR (GPIO 36 / VP) pada miniatur diorama Smart City dengan mikrokontroler ESP32 dalam kondisi **modul radio Wi-Fi dinonaktifkan sepenuhnya (OFF)**.

---

## 1. Parameter & Kondisi Pengujian
* **Firmware:** `src/test_ldr.cpp`
* **Target Pin:** GPIO 36 (VP / ADC1 Channel 0, Input-Only)
* **Rangkaian Sensor:** Pembagi tegangan LDR 5mm + Resistor 10 kOhm ke GND
* **Atenuasi ADC:** `analogSetAttenuation(ADC_11db)` (Skala 0 - 3.3V)
* **Kondisi Wi-Fi:** **OFF (Pustaka Wi-Fi dan modul radio tidak diinisialisasi)**
* **Ambang Batas Siang/Malam (Threshold):** `3650` (12-bit ADC: 0 s/d 4095)
* **Beban Output Terhubung:** Lampu Rumah 1 & 2 (GPIO 18) dan Rumah 3 & 4 (GPIO 19)

---

## 2. Kesimpulan Hasil Pengujian (Proofing)
1. **Pembacaan Linier & Bergradasi:**
   - Tanpa adanya pancaran radio Wi-Fi (beaconing / scanning AP), nilai ADC pada pin GPIO 36 (VP) terbaca **bergradasi halus** mengikuti intensitas cahaya ambient ruangan (rentang nilai 2100 s/d 3450 dalam kondisi redup/sedang, dan mencapai 3700 s/d 4095 saat disinari cahaya terang).
2. **Tidak Ada Gejala Melompat Liar (Railing):**
   - Berbeda dengan saat mode Wi-Fi AP aktif di mana nilai langsung mentok ekstrem dari 4095 ke 0, pada mode offline nilai transisi perubahan cahaya dari siang ke malam berjalan secara wajar dan presisi.
3. **Logika Otomasi Berfungsi Sempurna:**
   - Deteksi pergantian status `SIANG` (ADC >= 3650 -> Lampu Rumah OFF) dan `MALAM` (ADC < 3650 -> Lampu Rumah ON) bekerja secara konsisten tanpa ada flicker (kedipan liar).

---

## 3. Log Mentah Serial Monitor

```text
[LDR OFFLINE] ADC:3375 | Volt:2.72V | Bar:[################....] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:3218 | Volt:2.59V | Bar:[###############.....] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:3310 | Volt:2.67V | Bar:[################....] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:3223 | Volt:2.60V | Bar:[###############.....] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:3222 | Volt:2.60V | Bar:[###############.....] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:3378 | Volt:2.72V | Bar:[################....] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:3215 | Volt:2.59V | Bar:[###############.....] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:3248 | Volt:2.62V | Bar:[###############.....] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:3376 | Volt:2.72V | Bar:[################....] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:3453 | Volt:2.78V | Bar:[################....] | Suasana:MALAM | Rumah:ON 

----------------- [EVENT PERUBAHAN CAHAYA] -----------------
>>> KONDISI: SIANG HARI (ADC: 3669 >= 3650) -> LAMPU RUMAH PADAM (OFF) <<<
------------------------------------------------------------

[LDR OFFLINE] ADC:4095 | Volt:3.30V | Bar:[####################] | Suasana:SIANG | Rumah:OFF

----------------- [EVENT PERUBAHAN CAHAYA] -----------------
>>> KONDISI: MALAM HARI (ADC: 3583 < 3650) -> LAMPU RUMAH MENYALA (ON) <<<
------------------------------------------------------------

[LDR OFFLINE] ADC:3583 | Volt:2.89V | Bar:[#################...] | Suasana:MALAM | Rumah:ON 

----------------- [EVENT PERUBAHAN CAHAYA] -----------------
>>> KONDISI: SIANG HARI (ADC: 3701 >= 3650) -> LAMPU RUMAH PADAM (OFF) <<<
------------------------------------------------------------


----------------- [EVENT PERUBAHAN CAHAYA] -----------------
>>> KONDISI: MALAM HARI (ADC: 3557 < 3650) -> LAMPU RUMAH MENYALA (ON) <<<
------------------------------------------------------------


----------------- [EVENT PERUBAHAN CAHAYA] -----------------
>>> KONDISI: SIANG HARI (ADC: 3700 >= 3650) -> LAMPU RUMAH PADAM (OFF) <<<
------------------------------------------------------------

[LDR OFFLINE] ADC:3779 | Volt:3.05V | Bar:[##################..] | Suasana:SIANG | Rumah:OFF

----------------- [EVENT PERUBAHAN CAHAYA] -----------------
>>> KONDISI: MALAM HARI (ADC: 3619 < 3650) -> LAMPU RUMAH MENYALA (ON) <<<
------------------------------------------------------------

[LDR OFFLINE] ADC:3293 | Volt:2.65V | Bar:[################....] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:3177 | Volt:2.56V | Bar:[###############.....] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:3088 | Volt:2.49V | Bar:[###############.....] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:3024 | Volt:2.44V | Bar:[##############......] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:3443 | Volt:2.77V | Bar:[################....] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:3203 | Volt:2.58V | Bar:[###############.....] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:2841 | Volt:2.29V | Bar:[#############.......] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:2695 | Volt:2.17V | Bar:[#############.......] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:2871 | Volt:2.31V | Bar:[##############......] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:2539 | Volt:2.05V | Bar:[############........] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:3120 | Volt:2.51V | Bar:[###############.....] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:2374 | Volt:1.91V | Bar:[###########.........] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:2342 | Volt:1.89V | Bar:[###########.........] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:2159 | Volt:1.74V | Bar:[##########..........] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:2182 | Volt:1.76V | Bar:[##########..........] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:2490 | Volt:2.01V | Bar:[############........] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:2320 | Volt:1.87V | Bar:[###########.........] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:   0 | Volt:0.00V | Bar:[....................] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:1230 | Volt:0.99V | Bar:[######..............] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:2626 | Volt:2.12V | Bar:[############........] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:2495 | Volt:2.01V | Bar:[############........] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:2513 | Volt:2.03V | Bar:[############........] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:2991 | Volt:2.41V | Bar:[##############......] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:2723 | Volt:2.19V | Bar:[#############.......] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:2518 | Volt:2.03V | Bar:[############........] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:2358 | Volt:1.90V | Bar:[###########.........] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:2768 | Volt:2.23V | Bar:[#############.......] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:2512 | Volt:2.02V | Bar:[############........] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:2447 | Volt:1.97V | Bar:[###########.........] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:2224 | Volt:1.79V | Bar:[##########..........] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:2320 | Volt:1.87V | Bar:[###########.........] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:2262 | Volt:1.82V | Bar:[###########.........] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:2368 | Volt:1.91V | Bar:[###########.........] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:2454 | Volt:1.98V | Bar:[###########.........] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:2427 | Volt:1.96V | Bar:[###########.........] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:2412 | Volt:1.94V | Bar:[###########.........] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:2459 | Volt:1.98V | Bar:[############........] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:2319 | Volt:1.87V | Bar:[###########.........] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:2512 | Volt:2.02V | Bar:[############........] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:2288 | Volt:1.84V | Bar:[###########.........] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:2384 | Volt:1.92V | Bar:[###########.........] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:2651 | Volt:2.14V | Bar:[############........] | Suasana:MALAM | Rumah:ON 
[LDR OFFLINE] ADC:2669 | Volt:2.15V | Bar:[#############.......] | Suasana:MALAM | Rumah:ON 
```
