/**
 * =============================================================================
 * PROGRAM PENGUJIAN: KENDALI KECEPATAN MOTOR DINAMO DC (PWM GPIO 15)
 * Mikrokontroler : ESP32 Dev Module (DOIT DevKit v1 30-Pin)
 * Framework      : Arduino (PlatformIO)
 * Target Perangkat: Motor Driver L298N / MOSFET / Transistor Dinamo 5V (Soket [J])
 * =============================================================================
 *
 * ALOKASI PIN:
 * - Soket [J] Pin 1 : GPIO 15 -> Sinyal PWM Kendali Kecepatan Motor (IN1 / ENA)
 * - Soket [J] Pin 2 : GND     -> Ground bersama driver motor
 *
 * KONTROL DARI SERIAL MONITOR (115200 Baud):
 * - Ketik Angka 0 s/d 255  : Mengatur nilai duty cycle PWM secara presisi
 * - Ketik Persentase (misal: "50%", "75%") : Otomatis dikonversi ke PWM 0..255
 * - Perintah Cepat (Shortcut):
 *     '0' atau 's' : STOP / Matikan motor (PWM 0)
 *     '1'          : Kecepatan 20% (PWM 51)
 *     '2'          : Kecepatan 40% (PWM 102)
 *     '3'          : Kecepatan 60% (PWM 153)
 *     '4'          : Kecepatan 80% (PWM 204)
 *     '5' atau 'f' : Kecepatan 100% Penuh (PWM 255)
 *     '+'          : Naikkan kecepatan (+10)
 *     '-'          : Turunkan kecepatan (-10)
 *     't'          : Jalankan tes sapuan otomatis (Ramp Test 0% -> 100% -> 0%)
 * =============================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// Definisi Pin Kendali Motor
constexpr uint8_t PIN_MOTOR = 15; // GPIO 15 (Soket [J] Pin 1)

// Status Motor
uint8_t currentSpeed    = 0;       // Nilai PWM 0..255
bool    isRamping       = false;   // Flag pengujian sapuan otomatis
int     rampStep        = 5;
bool    isKickstarting  = false;   // Flag pengujian kickstart
unsigned long kickstartTime = 0;

// Buffer input Serial Monitor
String inputBuffer = "";

// Timer Serial Monitor
unsigned long lastStatusPrint = 0;
constexpr unsigned long STATUS_INTERVAL_MS = 1000;

WiFiManager wm;

void setMotorSpeed(uint8_t speed) {
    currentSpeed = speed;
    analogWrite(PIN_MOTOR, currentSpeed);

    int percent = (currentSpeed * 100) / 255;
    float estVoltage = (5.0f * currentSpeed) / 255.0f;

    Serial.println("\n----------------------------------------------------------------");
    Serial.printf(">>> MOTOR DISET: PWM = %3d | Kecepatan = %3d%% | Est. Daya = %.2f V <<<\n",
                  currentSpeed, percent, estVoltage);
    if (currentSpeed == 0) {
        Serial.println("    Status: [MOTOR BERHENTI / MATI]");
    } else {
        Serial.println("    Status: [MOTOR AKTIF BERPUTAR]");
    }
    Serial.println("----------------------------------------------------------------\n");
}

void triggerKickstart() {
    Serial.println("\n----------------------------------------------------------------");
    Serial.println(">>> [KICKSTART DINAMO] Semburan Starter Speed 5 (PWM 255) selama 300 ms! <<<");
    Serial.println("----------------------------------------------------------------");
    isKickstarting = true;
    kickstartTime = millis();
    currentSpeed = 255;
    analogWrite(PIN_MOTOR, currentSpeed);
}

void printHelp() {
    Serial.println();
    Serial.println("================================================================================");
    Serial.println("          PENGUJIAN KENDALI KECEPATAN DINAMO 5V (PWM GPIO 15)                   ");
    Serial.println("================================================================================");
    Serial.println("Perintah yang dapat diketik di Serial Monitor (pilih Newline / Carriage Return):");
    Serial.println("  * Angka 0 - 255 : Set langsung nilai PWM (contoh: '128', '200')");
    Serial.println("  * Persen        : Set persentase kecepatan (contoh: '50%', '75%')");
    Serial.println("  * '0' / 's'     : STOP / Matikan dinamo seketika (PWM 0)");
    Serial.println("  * '1' s/d '5'   : Presets: 1=20%, 2=40%, 3=60%, 4=80%, 5=100%");
    Serial.println("  * 'k'           : Uji Kickstart (Starter Speed 5 selama 300ms -> turun ke Level 3)");
    Serial.println("  * '+' / '-'     : Naikkan / turunkan kecepatan secara bertahap (+/- 10)");
    Serial.println("  * 't'           : Jalankan siklus tes sapuan otomatis (Ramp 0% -> 100% -> 0%)");
    Serial.println("  * 'h'           : Tampilkan panduan ini kembali");
    Serial.println("================================================================================");
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Inisialisasi Pin Motor
    pinMode(PIN_MOTOR, OUTPUT);
    analogWrite(PIN_MOTOR, 0); // Awal motor mati aman

    printHelp();

    // Inisialisasi WiFiManager & OTA (Non-blocking)
    wm.setConfigPortalBlocking(false);
    wm.setConfigPortalTimeout(0);
    wm.setConnectTimeout(8); // Timeout cepat 8 detik

    Serial.println("[WIFI] Mencoba konek ke WiFi tersimpan (Maks 8 detik)...");
    if (wm.autoConnect("SmartCity-Motor-Test")) {
        Serial.println("[WIFI] Berhasil terhubung!");
        Serial.print("[WIFI] IP ESP32: ");
        Serial.println(WiFi.localIP());

        ArduinoOTA.setHostname("smartcity-motor-test");
        ArduinoOTA.onStart([]() {
            Serial.println("\n[OTA] Memulai upload firmware...");
            analogWrite(PIN_MOTOR, 0); // Amankan motor saat flash
        });
        ArduinoOTA.begin();
        Serial.println("[OTA] ArduinoOTA Siap!");
    } else {
        Serial.println("[WIFI] Mode Offline aktif. Kontrol Serial Monitor langsung siap!");
    }

    setMotorSpeed(0); // Konfirmasi motor di posisi 0
}

void processSerialInput(String cmd) {
    cmd.trim();
    if (cmd.length() == 0) return;

    // Batalkan mode ramping otomatis jika user mengirim perintah manual
    if (isRamping && cmd != "t") {
        isRamping = false;
        Serial.println(">>> Mode Sapuan Otomatis Dibatalkan oleh input user. <<<");
    }

    // 1. Perintah satu karakter / pintasan
    if (cmd.equalsIgnoreCase("h") || cmd == "?") {
        printHelp();
    } else if (cmd.equalsIgnoreCase("s") || cmd == "0") {
        isKickstarting = false;
        setMotorSpeed(0);
    } else if (cmd.equalsIgnoreCase("k")) {
        triggerKickstart();
    } else if (cmd == "1") {
        setMotorSpeed(51);  // 20%
    } else if (cmd == "2") {
        setMotorSpeed(102); // 40%
    } else if (cmd == "3") {
        setMotorSpeed(153); // 60%
    } else if (cmd == "4") {
        setMotorSpeed(204); // 80%
    } else if (cmd == "5" || cmd.equalsIgnoreCase("f")) {
        setMotorSpeed(255); // 100%
    } else if (cmd == "+") {
        int next = (int)currentSpeed + 10;
        if (next > 255) next = 255;
        setMotorSpeed((uint8_t)next);
    } else if (cmd == "-") {
        int next = (int)currentSpeed - 10;
        if (next < 0) next = 0;
        setMotorSpeed((uint8_t)next);
    } else if (cmd.equalsIgnoreCase("t")) {
        isRamping = !isRamping;
        if (isRamping) {
            Serial.println("\n>>> [MEMULAI MODE SAPUAN OTOMATIS] Dinamo akan naik-turun berkala! (Ketik 's' untuk stop) <<<");
        } else {
            Serial.println("\n>>> [MODE SAPUAN OTOMATIS DIMATIKAN] <<<");
            setMotorSpeed(0);
        }
    } else if (cmd.endsWith("%")) {
        // 2. Format Persentase (contoh: "75%")
        String valStr = cmd.substring(0, cmd.length() - 1);
        int percent = valStr.toInt();
        if (percent < 0) percent = 0;
        if (percent > 100) percent = 100;
        uint8_t pwmVal = (uint8_t)((percent * 255) / 100);
        setMotorSpeed(pwmVal);
    } else {
        // 3. Format Angka Langsung 0..255
        int val = cmd.toInt();
        if (cmd == "0" || val > 0) {
            if (val < 0) val = 0;
            if (val > 255) val = 255;
            setMotorSpeed((uint8_t)val);
        } else {
            Serial.printf("Perintah tidak dikenali: '%s'. Ketik 'h' untuk bantuan.\n", cmd.c_str());
        }
    }
}

void loop() {
    unsigned long now = millis();

    // Background WiFi & OTA
    wm.process();
    if (WiFi.status() == WL_CONNECTED) {
        ArduinoOTA.handle();
    }

    // Baca input dari Serial Monitor
    while (Serial.available() > 0) {
        char inChar = (char)Serial.read();
        if (inChar == '\n' || inChar == '\r') {
            if (inputBuffer.length() > 0) {
                processSerialInput(inputBuffer);
                inputBuffer = "";
            }
        } else {
            inputBuffer += inChar;
        }
    }

    // Penanganan Transisi Kickstart (Starter 300ms -> Level 3)
    if (isKickstarting && (now - kickstartTime >= 300)) {
        isKickstarting = false;
        setMotorSpeed(153); // Turun ke Level 3 (60% / PWM 153)
        Serial.println(">>> [KICKSTART SELESAI] Dinamo stabil berputar di Level 3 (60% / PWM 153) <<<\n");
    }

    // Logika Sapuan Otomatis (Ramping Sweep Test)
    static unsigned long lastRampTime = 0;
    if (isRamping && (now - lastRampTime >= 50)) {
        lastRampTime = now;
        int nextSpeed = (int)currentSpeed + rampStep;
        if (nextSpeed >= 255) {
            nextSpeed = 255;
            rampStep  = -5; // Berbalik arah turun
        } else if (nextSpeed <= 0) {
            nextSpeed = 0;
            rampStep  = 5;  // Berbalik arah naik
        }
        currentSpeed = (uint8_t)nextSpeed;
        analogWrite(PIN_MOTOR, currentSpeed);
    }

    // Update Status Berkala ke Serial Monitor setiap 1000 ms
    if (now - lastStatusPrint >= STATUS_INTERVAL_MS) {
        lastStatusPrint = now;
        int percent = (currentSpeed * 100) / 255;
        float estVoltage = (5.0f * currentSpeed) / 255.0f;
        const char* modeStr = isRamping ? "[RAMPING AUTO]" : (currentSpeed == 0 ? "[STOP]" : "[MANUAL]");

        Serial.printf("[DINAMO] Mode: %-14s | Speed: %3d/255 (%3d%%) | Est. Daya: %.2fV | Ketik 'h' utk bantuan\n",
                      modeStr, currentSpeed, percent, estVoltage);
    }

    delay(5);
}

