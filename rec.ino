#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h> // For utility functions
const uint16_t kRecvPin = D5;
const uint16_t kCaptureBufferSize = 1024; // Usually sufficient, increase if you get overflow warnings
const uint8_t kTimeout = 50;             // Timeout in ms
IRrecv irrecv(kRecvPin, kCaptureBufferSize, kTimeout, true); // Enable LED feedback if wanted
decode_results results;   // Object to store decode results
void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(50); }
  Serial.println("\n\nESP8266 IR Receiver - RAW CAPTURE MODE");

  irrecv.enableIRIn(); // Start the receiver

  Serial.print("IR Receiver Pin (GPIO): "); Serial.println(kRecvPin);
  Serial.println("Ready to capture RAW IR signals...");
  Serial.println("Point your ORIGINAL remote at the sensor and press buttons.");
  Serial.println("----------------------------------------");
}
void loop() {
  if (irrecv.decode(&results)) {

    // Print the basic decoded info (for reference)
    Serial.print("Decoded ");
    Serial.print(typeToString(results.decode_type));
    Serial.print(" ("); Serial.print(results.bits, DEC); Serial.print(" bits): ");
    serialPrintUint64(results.value, HEX);
    Serial.println();
    Serial.println(resultToSourceCode(&results));
    Serial.println("----------------------------------------");
    irrecv.resume(); // Resume listening for the next signal
  }
}