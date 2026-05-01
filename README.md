# AC Controller using ESP8266 with IR

## Overview
An IR-based AC controller that replicates remote signals using ESP8266.

## Features
- Reverse engineered AC remote protocol
- Captures and decodes IR hex signals
- Controls:
  - Power ON/OFF
  - Temperature (16°C – 30°C)
  - Modes (Cool, Dry, Fan, Auto)

## Components
- ESP8266
- TSOP 38238 (IR Receiver)
- IR LED (Transmitter)
- Arduino IDE

## Working
1. Captured IR signals from AC remote
2. Extracted hex data stream
3. Decoded byte structure:
   - Mode
   - Temperature
   - Power state
4. Implemented checksum validation
5. Re-transmitted signals to control AC

## Applications
- Smart home automation
- IoT-based appliance control
