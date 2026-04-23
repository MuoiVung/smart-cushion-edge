# ErgoVita - Smart Cushion Edge (Firmware)

This repository contains the ESP32 firmware for the Smart Cushion system.

## 📂 Structure
-   `esp32_1_firmware/`: Controls the Left FSR cluster, Temperature sensor, and Vibration Motor.
-   `esp32_2_firmware/`: Controls the Middle and Right FSR clusters.
-   `esp32_secrets.h`: Shared network and MQTT credentials (WiFi SSID, Password, Fog Node IP).

## 🛠️ Setup
1.  Open the `.ino` files in Arduino IDE or VS Code (PlatformIO).
2.  Install required libraries:
    -   `PubSubClient` (MQTT)
    -   `ArduinoJson`
3.  Configure `esp32_secrets.h` with your local WiFi and Fog Node IP.
4.  Flash `esp32_1_firmware` to the primary ESP32.
5.  Flash `esp32_2_firmware` to the secondary ESP32.

## 📡 Data Flow (Interface 01 & 05)
-   **Output**: Raw FSR ADC values (0-4095) sent to `cushion/raw` topic every 0.5s.
-   **Input**: Commands received on `cushion/command` topic to trigger vibration alerts.
