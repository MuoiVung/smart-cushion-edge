# smart-cushion-edge

ESP32 firmware for the Smart Cushion edge devices. This repository contains the Arduino sketches for two ESP32 nodes that collect pressure sensor data and transmit it via MQTT to the fog node.

## Structure

```
smart-cushion-edge/
├── esp32_1_firmware/          # Firmware for ESP32 Node 1 (seat sensors)
│   └── esp32_1_firmware.ino
├── esp32_2_firmware/          # Firmware for ESP32 Node 2 (back sensors)
│   └── esp32_2_firmware.ino
├── esp32_secrets.h.template   # WiFi & MQTT credentials template
└── .gitignore
```

## Setup

1. Copy `esp32_secrets.h.template` → `esp32_secrets.h`
2. Fill in your WiFi SSID, password, and MQTT broker IP in `esp32_secrets.h`
3. Open the desired firmware folder in Arduino IDE
4. Select **ESP32 Dev Module** as the board
5. Install required libraries:
   - `PubSubClient` (MQTT)
   - `ArduinoJson`
   - `WiFi` (built-in)
6. Upload to the ESP32 device

## Dependencies

- Arduino IDE 2.x
- ESP32 board package
- PubSubClient
- ArduinoJson
