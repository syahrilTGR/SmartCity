/**
 * =============================================================================
 * PROGRAM PENGUJIAN: SIMULATOR 10-PORT BANANA / BNC + LPJ JALUR BAWAH (PWM)
 * Mikrokontroler : ESP32 Dev Module (DOIT DevKit v1 30-Pin)
 * Framework      : Arduino (PlatformIO)
 * Fitur          : Pemindaian Matriks Dinamis, Nodal Solver (DFS), Deteksi Korslet,
 *                  Output Kecerahan PWM (Hukum Pembagi Tegangan),
 *                  Update Serial Monitor Berkala (500ms) + Event Log Instan.
 * =============================================================================
 *
 * ALOKASI PIN FISIK SESUAI SOKET PCB MATRIX (docs/pcb_matrix_layout_10port.svg):
 * 1. Port Banana Beban Lampu (8 Port):
 *    - Soket [O] (Eks BNC_SERI) : Pin 1 = D4 (A1),  Pin 3 = D25 (A2)
 *    - Soket [N] (Eks BNC_PAR)  : Pin 1 = RX2 (B1), Pin 2 = D21 (C1), Pin 3 = TX2 (B2)
 *    - Soket [K] (BNC_EXT)      : Pin 1 = D18 (D1), Pin 2 = D19 (D2), Pin 3 = D22 (C2)
 *
 * 2. Rel Sumber Daya (2 Port):
 *    - Soket [O] Pin 2 : SRC (+) -> Rel 3.3V (via Resistor 1 kOhm)
 *    - Soket [O] Pin 4 : SRC (-) -> Rel Ground (GND)
 *
 * 3. Output 4 Lampu Jalan Miniatur Jalur Bawah (PWM):
 *    - Lampu 1 (Pasangan A1 & A2) : GPIO 26 (Soket [H])
 *    - Lampu 2 (Pasangan B1 & B2) : GPIO 12 (Soket [M])
 *    - Lampu 3 (Pasangan C1 & C2) : GPIO 33 (Soket [D] Pin 3)
 *    - Lampu 4 (Pasangan D1 & D2) : GPIO 32 (Soket [D] Pin 1)
 * =============================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// =============================================================================
// DEFINISI 8 PORT BANANA / BNC & 4 OUTPUT LPJ BAWAH
// =============================================================================
enum BananaPortIndex {
    PORT_A1 = 0, // Indeks 0: GPIO 4  (Soket [O] Pin 1) -> Beban Lampu 1
    PORT_A2,     // Indeks 1: GPIO 25 (Soket [O] Pin 3) -> Beban Lampu 1
    PORT_B1,     // Indeks 2: GPIO 16 (Soket [N] Pin 1) -> Beban Lampu 2
    PORT_B2,     // Indeks 3: GPIO 17 (Soket [N] Pin 3) -> Beban Lampu 2
    PORT_C1,     // Indeks 4: GPIO 21 (Soket [N] Pin 2) -> Beban Lampu 3
    PORT_C2,     // Indeks 5: GPIO 22 (Soket [K] Pin 3) -> Beban Lampu 3
    PORT_D1,     // Indeks 6: GPIO 18 (Soket [K] Pin 1) -> Beban Lampu 4
    PORT_D2,     // Indeks 7: GPIO 19 (Soket [K] Pin 2) -> Beban Lampu 4
    PORT_COUNT = 8
};

const char* PORT_SHORT_NAMES[PORT_COUNT] = {
    "A1", "A2", "B1", "B2", "C1", "C2", "D1", "D2"
};

const char* PORT_FULL_NAMES[PORT_COUNT] = {
    "A1 (GPIO 4 / Soket O-1)", "A2 (GPIO 25 / Soket O-3)",
    "B1 (GPIO 16 / Soket N-1)", "B2 (GPIO 17 / Soket N-3)",
    "C1 (GPIO 21 / Soket N-2)", "C2 (GPIO 22 / Soket K-3)",
    "D1 (GPIO 18 / Soket K-1)", "D2 (GPIO 19 / Soket K-2)"
};

constexpr uint8_t BANANA_PINS[PORT_COUNT] = {
    4,   // Index 0: PORT_A1 (Soket [O] Pin 1)
    25,  // Index 1: PORT_A2 (Soket [O] Pin 3)
    16,  // Index 2: PORT_B1 (Soket [N] Pin 1, RX2)
    17,  // Index 3: PORT_B2 (Soket [N] Pin 3, TX2)
    21,  // Index 4: PORT_C1 (Soket [N] Pin 2)
    22,  // Index 5: PORT_C2 (Soket [K] Pin 3)
    18,  // Index 6: PORT_D1 (Soket [K] Pin 1)
    19   // Index 7: PORT_D2 (Soket [K] Pin 2)
};

constexpr uint8_t PIN_LPJ_BWH_1 = 26; // LPJ Bawah 1 (Soket [H])
constexpr uint8_t PIN_LPJ_BWH_2 = 12; // LPJ Bawah 2 (Soket [M])
constexpr uint8_t PIN_LPJ_BWH_3 = 33; // LPJ Bawah 3 (Soket [D] Pin 3)
constexpr uint8_t PIN_LPJ_BWH_4 = 32; // LPJ Bawah 4 (Soket [D] Pin 1)

constexpr uint8_t LPJ_BWH_PINS[4] = {
    PIN_LPJ_BWH_1,
    PIN_LPJ_BWH_2,
    PIN_LPJ_BWH_3,
    PIN_LPJ_BWH_4
};

// =============================================================================
// STRUKTUR DATA & STATE SIMULATOR
// =============================================================================
constexpr uint8_t NODE_COUNT = 10;
constexpr uint8_t NODE_VCC   = 8; // Rel Sumber Daya (+) 3.3V
constexpr uint8_t NODE_GND   = 9; // Rel Sumber Daya (-) GND

struct DSU {
    uint8_t parent[NODE_COUNT];
    void init() {
        for (uint8_t i = 0; i < NODE_COUNT; i++) parent[i] = i;
    }
    uint8_t find(uint8_t x) {
        if (parent[x] == x) return x;
        return parent[x] = find(parent[x]);
    }
    void unite(uint8_t a, uint8_t b) {
        uint8_t rootA = find(a);
        uint8_t rootB = find(b);
        if (rootA != rootB) parent[rootB] = rootA;
    }
};

// Objek global DSU terkini untuk kebutuhan debugging
DSU lastDsuState;

bool isSrcShortCircuited = false;
uint8_t lpjBwhDuty[4]    = {0, 0, 0, 0}; // Duty PWM 0..255
uint8_t lpjBwhChain[4]   = {0, 0, 0, 0}; // Panjang rantai seri (1=Paralel, 2=2 Seri, dst)

// Hasil pembacaan fisik Dual-Read untuk keperluan debug
uint8_t rawValPU[PORT_COUNT];
uint8_t rawValPD[PORT_COUNT];

// State riwayat untuk deteksi perubahan seketika (Fast Event Log)
uint8_t lastDuty[4]      = {255, 255, 255, 255};
bool    lastShort        = false;
char    lastNodeSig[64]  = "";

// Timer interval Serial Monitor 500ms
unsigned long lastPrintTime = 0;
constexpr unsigned long PRINT_INTERVAL_MS = 500;

WiFiManager wm;

// =============================================================================
// ALGORITMA PENELUSURAN RANGKAIAN (DFS TRAVERSAL)
// =============================================================================
void searchCircuitPaths(uint8_t currentNet, uint8_t targetNet, uint8_t visitedLamps, uint8_t currentLength,
                        uint8_t netOfLamp[4][2], uint8_t minChain[4]) {
    if (currentNet == targetNet) {
        // Sirkuit tertutup dari VCC ke GND berhasil terbentuk!
        for (uint8_t k = 0; k < 4; k++) {
            if (visitedLamps & (1 << k)) {
                if (currentLength < minChain[k]) {
                    minChain[k] = currentLength;
                }
            }
        }
        return;
    }

    for (uint8_t k = 0; k < 4; k++) {
        if (!(visitedLamps & (1 << k))) {
            uint8_t nA = netOfLamp[k][0];
            uint8_t nB = netOfLamp[k][1];

            // Abaikan lampu yang kaki-kakinya terkorslet sendiri (nA == 255)
            if (nA == 255 || nB == 255) continue;

            if (nA == currentNet) {
                searchCircuitPaths(nB, targetNet, visitedLamps | (1 << k), currentLength + 1, netOfLamp, minChain);
            } else if (nB == currentNet) {
                searchCircuitPaths(nA, targetNet, visitedLamps | (1 << k), currentLength + 1, netOfLamp, minChain);
            }
        }
    }
}

// =============================================================================
// PEMINDAIAN MATRIKS DINAMIS & NODAL SOLVER
// =============================================================================
void scanBananaMatrix() {
    DSU dsu;
    dsu.init();

    // 1. Uji koneksi langsung ke SRC (+) [3.3V] dan SRC (-) [GND] via Synchronous Dual-Read
    //    KRUSIAL: Seluruh 8 port wajib di-set ke INPUT_PULLUP secara serempak, lalu
    //    di-set ke INPUT_PULLDOWN secara serempak!
    //    Jika dilakukan satu-per-satu di dalam satu loop, port pasangan yang terhubung
    //    kabel jumper masih berstatus INPUT_PULLUP saat port ini diuji INPUT_PULLDOWN,
    //    sehingga menarik tegangan naik (cross-pulling) dan salah terbaca sebagai VCC!

    // 1A. Fase Pull-Up Serempak (Semua 8 Port = INPUT_PULLUP)
    for (uint8_t i = 0; i < PORT_COUNT; i++) {
        pinMode(BANANA_PINS[i], INPUT_PULLUP);
    }
    delayMicroseconds(30);
    for (uint8_t i = 0; i < PORT_COUNT; i++) {
        rawValPU[i] = digitalRead(BANANA_PINS[i]);
    }

    // 1B. Fase Pull-Down Serempak (Semua 8 Port = INPUT_PULLDOWN)
    for (uint8_t i = 0; i < PORT_COUNT; i++) {
        pinMode(BANANA_PINS[i], INPUT_PULLDOWN);
    }
    delayMicroseconds(30);
    for (uint8_t i = 0; i < PORT_COUNT; i++) {
        rawValPD[i] = digitalRead(BANANA_PINS[i]);
    }

    // 1C. Kembalikan semua 8 port ke INPUT_PULLUP & evaluasi koneksi ke SRC(+) / SRC(-)
    for (uint8_t i = 0; i < PORT_COUNT; i++) {
        pinMode(BANANA_PINS[i], INPUT_PULLUP);
        if (rawValPU[i] == HIGH && rawValPD[i] == HIGH) {
            dsu.unite(i, NODE_VCC); // Terhubung ke SRC (+) 3.3V
        } else if (rawValPU[i] == LOW && rawValPD[i] == LOW) {
            dsu.unite(i, NODE_GND); // Terhubung ke SRC (-) GND
        }
    }
    delayMicroseconds(30);

    uint8_t rootVCC = dsu.find(NODE_VCC);
    uint8_t rootGND = dsu.find(NODE_GND);

    // 2. Uji jumper antar-port yang mengambang (Floating Intermediate Jumpers)
    // KRUSIAL: HANYA uji pin yang TIDAK terhubung ke GND atau VCC.
    // Jika pin yang tercolok ke GND ikut dibaca, pin tersebut akan selalu terbaca LOW
    // dan menyebabkan port lain salah di-unite ke Ground!
    for (uint8_t i = 0; i < PORT_COUNT; i++) {
        // Lewati pin yang sudah terhubung langsung ke VCC atau GND
        if (dsu.find(i) == rootVCC || dsu.find(i) == rootGND) continue;

        pinMode(BANANA_PINS[i], OUTPUT);
        digitalWrite(BANANA_PINS[i], LOW);
        delayMicroseconds(30);

        for (uint8_t j = i + 1; j < PORT_COUNT; j++) {
            // Hanya periksa pin j yang juga berstatus mengambang (floating)
            if (dsu.find(j) == rootVCC || dsu.find(j) == rootGND) continue;

            if (digitalRead(BANANA_PINS[j]) == LOW) {
                dsu.unite(i, j); // Port i dan j terhubung kabel jumper!
            }
        }

        // Kembalikan ke mode default INPUT_PULLUP
        pinMode(BANANA_PINS[i], INPUT_PULLUP);
        delayMicroseconds(30);
    }

    // Salin objek DSU terkini untuk laporan telemetri debug
    lastDsuState = dsu;

    // 3. Deteksi Korslet Rel Utama (SRC+ dicolok langsung ke SRC-)
    isSrcShortCircuited = (dsu.find(NODE_VCC) == dsu.find(NODE_GND));
    if (isSrcShortCircuited) {
        for (uint8_t k = 0; k < 4; k++) {
            lpjBwhChain[k] = 0;
            lpjBwhDuty[k]  = 0;
        }
        return;
    }

    // 4. Analisis Graf untuk 4 Beban Lampu
    uint8_t netVCC = dsu.find(NODE_VCC);
    uint8_t netGND = dsu.find(NODE_GND);

    uint8_t netOfLamp[4][2];
    uint8_t minChain[4] = {255, 255, 255, 255};

    for (uint8_t k = 0; k < 4; k++) {
        uint8_t p1 = 2 * k;     // Kaki 1 (A1, B1, C1, D1)
        uint8_t p2 = 2 * k + 1; // Kaki 2 (A2, B2, C2, D2)
        netOfLamp[k][0] = dsu.find(p1);
        netOfLamp[k][1] = dsu.find(p2);

        // Jika kedua kaki beban berada di net yang sama, beban terkorslet sendiri (shorted load)
        if (netOfLamp[k][0] == netOfLamp[k][1]) {
            netOfLamp[k][0] = 255; // Tandai tidak dapat dilalui arus
            netOfLamp[k][1] = 255;
        }
    }

    // Jalankan DFS mencari rantai seri terpendek dari netVCC ke netGND
    searchCircuitPaths(netVCC, netGND, 0, 0, netOfLamp, minChain);

    // 5. Tentukan Kecerahan PWM & Rantai Seri Masing-Masing Lampu
    for (uint8_t k = 0; k < 4; k++) {
        if (minChain[k] == 1) {
            lpjBwhChain[k] = 1;
            lpjBwhDuty[k]  = 255; // 100% (Paralel / 1 Lampu Mandiri)
        } else if (minChain[k] == 2) {
            lpjBwhChain[k] = 2;
            lpjBwhDuty[k]  = 128; // 50% (2 Lampu Seri)
        } else if (minChain[k] == 3) {
            lpjBwhChain[k] = 3;
            lpjBwhDuty[k]  = 85;  // 33% (3 Lampu Seri)
        } else if (minChain[k] == 4) {
            lpjBwhChain[k] = 4;
            lpjBwhDuty[k]  = 64;  // 25% (4 Lampu Seri)
        } else {
            lpjBwhChain[k] = 0;
            lpjBwhDuty[k]  = 0;   // 0% (Padam / Putus / Short)
        }
    }
}

// =============================================================================
// HELPER LABEL NODAL DEBUGGING
// =============================================================================
void getPortStatusLabel(uint8_t idx, char* buf, size_t maxLen) {
    uint8_t root = lastDsuState.find(idx);
    if (root == lastDsuState.find(NODE_VCC)) {
        snprintf(buf, maxLen, "VCC");
    } else if (root == lastDsuState.find(NODE_GND)) {
        snprintf(buf, maxLen, "GND");
    } else {
        // Cari apakah terhubung ke port lain
        int matchIdx = -1;
        for (uint8_t j = 0; j < PORT_COUNT; j++) {
            if (j != idx && lastDsuState.find(j) == root) {
                matchIdx = j;
                break;
            }
        }
        if (matchIdx != -1) {
            snprintf(buf, maxLen, "=%s", PORT_SHORT_NAMES[matchIdx]);
        } else {
            snprintf(buf, maxLen, "---"); // Open / Floating sendirian
        }
    }
}

// =============================================================================
// SETUP
// =============================================================================
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("================================================================================");
    Serial.println("         PROGRAM PENGUJIAN & DIAGNOSTIK SIMULATOR 10-PORT BNC ESP32             ");
    Serial.println("================================================================================");
    Serial.println("Panduan Kode Status Port:");
    Serial.println("  [VCC] : Tercolok ke terminal SRC (+) 3.3V");
    Serial.println("  [GND] : Tercolok ke terminal SRC (-) Ground");
    Serial.println("  [=XX] : Terjumper ke port XX (misal: A2 berisi '=B1' berarti A2 dijumper ke B1)");
    Serial.println("  [---] : Terbuka (Open circuit / tidak tercolok kabel)");
    Serial.println("--------------------------------------------------------------------------------");

    // 1. Inisialisasi 8 Port Banana sebagai INPUT_PULLUP
    for (uint8_t i = 0; i < PORT_COUNT; i++) {
        pinMode(BANANA_PINS[i], INPUT_PULLUP);
    }

    // 2. Inisialisasi 4 Output Lampu Miniatur LPJ Bawah
    for (uint8_t k = 0; k < 4; k++) {
        pinMode(LPJ_BWH_PINS[k], OUTPUT);
        analogWrite(LPJ_BWH_PINS[k], 0); // Awal padam
    }

    Serial.println("[I/O] 8 Port Banana (A1..D2) dan 4 Output Lampu LPJ Bawah siap!");

    // 3. Inisialisasi WiFiManager Non-Blocking
    wm.setConfigPortalBlocking(false);
    wm.setConfigPortalTimeout(0);
    wm.setConnectTimeout(8); // Timeout cepat 8 detik

    Serial.println("[WIFI] Mencoba konek ke WiFi tersimpan (Maks 8 detik)...");
    if (wm.autoConnect("SmartCity-BNC-Test")) {
        Serial.println("[WIFI] Berhasil terhubung!");
        Serial.print("[WIFI] IP ESP32: ");
        Serial.println(WiFi.localIP());

        ArduinoOTA.setHostname("smartcity-bnc-test");
        ArduinoOTA.onStart([]() {
            Serial.println("\n[OTA] Memulai upload firmware...");
            for (uint8_t k = 0; k < 4; k++) analogWrite(LPJ_BWH_PINS[k], 0);
        });
        ArduinoOTA.begin();
        Serial.println("[OTA] ArduinoOTA Siap!");
    } else {
        Serial.println("[WIFI] Mode Offline aktif. Pengujian simulator BNC langsung berjalan!");
    }

    Serial.println("================================================================================");
    Serial.println("Mulai pengujian... Silakan pasang kabel jumper pada port BNC!");
    Serial.println("================================================================================\n");
}

// =============================================================================
// LOOP UTAMA
// =============================================================================
void loop() {
    unsigned long now = millis();

    // 0. Proses WiFiManager & OTA di latar belakang
    wm.process();
    if (WiFi.status() == WL_CONNECTED) {
        ArduinoOTA.handle();
    }

    // Reset Wi-Fi jika user ketik 'r' di Serial Monitor
    if (Serial.available()) {
        char c = Serial.read();
        if (c == 'r' || c == 'R') {
            Serial.println("\n[WIFI] Mereset pengaturan Wi-Fi...");
            WiFiManager wm;
            wm.resetSettings();
            delay(1000);
            ESP.restart();
        }
    }

    // 1. Pemindaian Matriks BNC secara berkala setiap 35 ms
    static unsigned long lastScanTime = 0;
    if (now - lastScanTime >= 35) {
        lastScanTime = now;
        scanBananaMatrix();

        // Terapkan output PWM ke fisik 4 lampu
        for (uint8_t k = 0; k < 4; k++) {
            uint8_t targetDuty = (!isSrcShortCircuited) ? lpjBwhDuty[k] : 0;
            analogWrite(LPJ_BWH_PINS[k], targetDuty);
        }
    }

    // 2. Deteksi Perubahan Seketika (Fast Event Log saat kabel dicolok / dicabut)
    char curNodeSig[64] = "";
    for (uint8_t i = 0; i < PORT_COUNT; i++) {
        char st[8];
        getPortStatusLabel(i, st, sizeof(st));
        strncat(curNodeSig, st, sizeof(curNodeSig) - strlen(curNodeSig) - 1);
        strncat(curNodeSig, "|", sizeof(curNodeSig) - strlen(curNodeSig) - 1);
    }

    bool dutyChanged  = (lpjBwhDuty[0] != lastDuty[0] || lpjBwhDuty[1] != lastDuty[1] ||
                         lpjBwhDuty[2] != lastDuty[2] || lpjBwhDuty[3] != lastDuty[3]);
    bool shortChanged = (isSrcShortCircuited != lastShort);
    bool nodeChanged  = (strcmp(curNodeSig, lastNodeSig) != 0);

    if (dutyChanged || shortChanged || nodeChanged) {
        lastDuty[0] = lpjBwhDuty[0];
        lastDuty[1] = lpjBwhDuty[1];
        lastDuty[2] = lpjBwhDuty[2];
        lastDuty[3] = lpjBwhDuty[3];
        lastShort   = isSrcShortCircuited;
        strncpy(lastNodeSig, curNodeSig, sizeof(lastNodeSig) - 1);
        lastNodeSig[sizeof(lastNodeSig) - 1] = '\0';

        Serial.println("\n================================================================================");
        Serial.println(">>> [EVENT PERUBAHAN KABEL TERDETEKSI] <<<");

        if (isSrcShortCircuited) {
            Serial.println("!!! [BAHAYA] KORSLET LANGSUNG ANTARA SRC(+) DAN SRC(-) TERDETEKSI! !!!");
            Serial.println("    Seluruh lampu dipadamkan seketika demi proteksi sirkuit.");
        } else {
            Serial.println("--- STATUS OUTPUT LAMPU ---");
            const char* lampLabels[4] = {
                "L1 [D26] (Beban A: A1 - A2)",
                "L2 [D12] (Beban B: B1 - B2)",
                "L3 [D33] (Beban C: C1 - C2)",
                "L4 [D32] (Beban D: D1 - D2)"
            };

            for (uint8_t k = 0; k < 4; k++) {
                if (lpjBwhChain[k] == 1) {
                    Serial.printf("  * %s : MENYALA TERANG (100%% - PWM 255) | Mandiri / Paralel\n", lampLabels[k]);
                } else if (lpjBwhChain[k] == 2) {
                    Serial.printf("  * %s : MENYALA REDUP  ( 50%% - PWM 128) | 2 Lampu Seri\n", lampLabels[k]);
                } else if (lpjBwhChain[k] == 3) {
                    Serial.printf("  * %s : MENYALA SANGAT REDUP ( 33%% - PWM  85) | 3 Lampu Seri\n", lampLabels[k]);
                } else if (lpjBwhChain[k] == 4) {
                    Serial.printf("  * %s : MENYALA MINIMAL ( 25%% - PWM  64) | 4 Lampu Seri\n", lampLabels[k]);
                } else {
                    Serial.printf("  * %s : PADAM ( 0%% - PWM   0) | Terbuka / Korslet Beban\n", lampLabels[k]);
                }
            }

            Serial.println("\n--- DIAGNOSTIK KONEKSI SETIAP PORT ---");
            for (uint8_t i = 0; i < PORT_COUNT; i++) {
                char nodeStatus[16];
                getPortStatusLabel(i, nodeStatus, sizeof(nodeStatus));

                const char* desc = "Terbuka (Tidak dicolok)";
                if (strcmp(nodeStatus, "VCC") == 0) desc = "Tercolok ke SRC (+) 3.3V";
                else if (strcmp(nodeStatus, "GND") == 0) desc = "Tercolok ke SRC (-) Ground";
                else if (nodeStatus[0] == '=') desc = "Terhubung jumper";

                Serial.printf("  - Port %-25s : Node=[%-4s] | Dual-Read [PU:%d, PD:%d] => %s\n",
                              PORT_FULL_NAMES[i], nodeStatus, rawValPU[i], rawValPD[i], desc);
            }
        }
        Serial.println("================================================================================\n");
    }

    // 3. Update Status Ringkas Berkala ke Serial Monitor Setiap 500 ms
    if (now - lastPrintTime >= PRINT_INTERVAL_MS) {
        lastPrintTime = now;

        if (isSrcShortCircuited) {
            Serial.printf("[BNC 500ms] STATUS: [!KORSLET REL UTAMA!] | L1..L4 PADAM PROTEKSI | IP: %s\n",
                          (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString().c_str() : "OFFLINE");
        } else {
            // Siapkan label ringkas status node untuk ke-8 port
            char st[PORT_COUNT][8];
            for (uint8_t i = 0; i < PORT_COUNT; i++) {
                getPortStatusLabel(i, st[i], sizeof(st[i]));
            }

            Serial.printf("[BNC 500ms] L1: %2d%% (P:%3d) | L2: %2d%% (P:%3d) | L3: %2d%% (P:%3d) | L4: %2d%% (P:%3d) || A1:%-3s A2:%-3s | B1:%-3s B2:%-3s | C1:%-3s C2:%-3s | D1:%-3s D2:%-3s\n",
                          (lpjBwhDuty[0] * 100) / 255, lpjBwhDuty[0],
                          (lpjBwhDuty[1] * 100) / 255, lpjBwhDuty[1],
                          (lpjBwhDuty[2] * 100) / 255, lpjBwhDuty[2],
                          (lpjBwhDuty[3] * 100) / 255, lpjBwhDuty[3],
                          st[0], st[1], st[2], st[3], st[4], st[5], st[6], st[7]);
        }
    }

    delay(5); // Loop sampling interval ringan (200 FPS) untuk kestabilan RTOS
}
