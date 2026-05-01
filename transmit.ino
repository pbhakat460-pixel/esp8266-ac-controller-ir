#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>

// --- Pin Configuration (ESP8266 GPIO numbers) ---
const uint16_t kIrLed      = 4;   // GPIO 4 (D2) -> IR LED (via transistor!)
const uint16_t btnUpPin    = 5;   // GPIO 5 (D1) -> Temp UP
const uint16_t btnDownPin  = 14;  // GPIO 14 (D5) -> Temp DOWN
const uint16_t btnModePin  = 12;  // GPIO 12 (D6) -> MODE

IRsend irsend(kIrLed);

// --- Constants ---
const uint8_t SS_CMD_LENGTH = 10;   // Voltas frame length

// --- State Variables ---
bool   acIsPowerOn        = false;
String acCurrentMode      = "COOL";    // COOL, DRY, AUTO, FAN
int    acCurrentTemp      = 24;        // 16 - 30
String acCurrentFanSpeed  = "LOW";     // (not used in frame yet)

// --- Lookup Table for Base Temperature Codes ---
const uint8_t tempCodes[] = {
  0x10, // 16
  0x11, // 17
  0x12, // 18
  0x13, // 19
  0x14, // 20
  0x15, // 21
  0x16, // 22
  0x17, // 23
  0x18, // 24
  0x19, // 25
  0x1A, // 26
  0x1B, // 27
  0x1C, // 28
  0x1D, // 29
  0x1E  // 30   <-- missing comma fixed
};

// --- Function Prototype ---
void sendConfiguredVoltasIR();

void setup() {
  Serial.begin(115200);
  irsend.begin();  // Required for IRremoteESP8266 on ESP8266

  pinMode(btnUpPin,   INPUT_PULLUP);
  pinMode(btnDownPin, INPUT_PULLUP);
  pinMode(btnModePin, INPUT_PULLUP);

  Serial.println();
  Serial.println(F("--- Voltas AC Controller Ready ---"));
  Serial.println(F("Controls: D1=UP, D5=DOWN, D6=MODE"));
}

void loop() {
  // 1. Button: Temp UP
  if (digitalRead(btnUpPin) == LOW) {
    delay(50);  // Debounce
    if (digitalRead(btnUpPin) == LOW) {
      acIsPowerOn = true;
      if (acCurrentTemp < 30) acCurrentTemp++;

      Serial.println(F("[BTN] UP Pressed"));
      sendConfiguredVoltasIR();

      while (digitalRead(btnUpPin) == LOW) { delay(10); }
      delay(200);
    }
  }

  // 2. Button: Temp DOWN
  if (digitalRead(btnDownPin) == LOW) {
    delay(50);
    if (digitalRead(btnDownPin) == LOW) {
      acIsPowerOn = true;
      if (acCurrentTemp > 16) acCurrentTemp--;

      Serial.println(F("[BTN] DOWN Pressed"));
      sendConfiguredVoltasIR();

      while (digitalRead(btnDownPin) == LOW) { delay(10); }
      delay(200);
    }
  }

  // 3. Button: MODE Change
  if (digitalRead(btnModePin) == LOW) {
    delay(50);
    if (digitalRead(btnModePin) == LOW) {
      acIsPowerOn = true;

      // COOL -> DRY -> AUTO -> FAN -> COOL
      if      (acCurrentMode == "COOL") acCurrentMode = "DRY";
      else if (acCurrentMode == "DRY")  acCurrentMode = "AUTO";
      else if (acCurrentMode == "AUTO") acCurrentMode = "FAN";
      else                              acCurrentMode = "COOL";

      Serial.println(F("[BTN] MODE Pressed"));
      sendConfiguredVoltasIR();

      while (digitalRead(btnModePin) == LOW) { delay(10); }
      delay(200);
    }
  }
}

// ---------------------------------------------------------
//       Build 10-byte Voltas frame and send via IR
// ---------------------------------------------------------
void sendConfiguredVoltasIR() {
  uint8_t commandToSend[SS_CMD_LENGTH];

  bool   p = acIsPowerOn;
  String m = acCurrentMode;
  int    t = acCurrentTemp;

  // 1. Temperature limiter for DRY / AUTO
  if (m.equalsIgnoreCase("DRY") || m.equalsIgnoreCase("AUTO")) {
    if (t < 24) t = 24;
  }

  Serial.println(F("------------------------------------------------"));
  Serial.printf("[STATUS] Power: %s | Mode: %s | Temp (Sent): %d\n",
                p ? "ON" : "OFF", m.c_str(), t);

  // 2. Build bytes 0..8
  if (!p) {
    // Power OFF fixed frame (tweak if your sniffed data differs)
    commandToSend[0] = 0x63;
    commandToSend[1] = 0x48;
    commandToSend[2] = 0x00;
    commandToSend[3] = 0x10;
    commandToSend[4] = 0x3B;
    commandToSend[5] = 0x3B;
    commandToSend[6] = 0x3B;
    commandToSend[7] = 0x11;
    commandToSend[8] = 0x00;
  } else {
    // Header
    commandToSend[0] = 0x63;

    // Byte 1: Mode mapping (based on your reverse-engineering)
    if (m.equalsIgnoreCase("DRY")) {
      commandToSend[1] = 0x84;
    } else if (m.equalsIgnoreCase("FAN")) {
      commandToSend[1] = 0x81;
    } else if (m.equalsIgnoreCase("AUTO")) {
      commandToSend[1] = 0x30;
    } else {
      commandToSend[1] = 0x28;  // COOL default
    }

    // Byte 2: Fixed
    commandToSend[2] = 0x80;

    // Byte 3: Temperature (with mode bits)
    int tempIndex       = constrain(t, 16, 30) - 16;
    uint8_t baseTempCode = tempCodes[tempIndex];

    if (m.equalsIgnoreCase("COOL")) {
      commandToSend[3] = baseTempCode;
    } else if (m.equalsIgnoreCase("DRY")) {
      commandToSend[3] = baseTempCode | 0x04;
    } else if (m.equalsIgnoreCase("AUTO")) {
      commandToSend[3] = baseTempCode | 0x08;
    } else if (m.equalsIgnoreCase("FAN")) {
      commandToSend[3] = 0xE4;  // Fan-only value (from your mapping)
    } else {
      commandToSend[3] = baseTempCode;
    }

    // Bytes 4–8: fixed pattern
    commandToSend[4] = 0x3B;
    commandToSend[5] = 0x3B;
    commandToSend[6] = 0x3B;
    commandToSend[7] = 0x11;
    commandToSend[8] = 0x00;
  }

  // 3. Checksum: sum(0..8), inverted → byte 9
  uint8_t sum = 0;
  for (int i = 0; i < 9; i++) sum += commandToSend[i];
  commandToSend[9] = ~sum;

  // Debug: print full frame
  Serial.print(F("[OUTPUT] Hex Stream: "));
  for (int i = 0; i < SS_CMD_LENGTH; i++) {
    Serial.printf("0x%02X ", commandToSend[i]);
  }
  Serial.println();

  Serial.println(F("[OUTPUT] Sending IR (Voltas protocol)..."));
  // This uses the library's dedicated Voltas sender (80 bits / 10 bytes)
  irsend.sendVoltas(commandToSend, SS_CMD_LENGTH);
}
