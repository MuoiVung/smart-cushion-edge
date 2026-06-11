# 📦 ErgoVita - Smart Cushion Edge

### Hardware Sensing & Edge Telemetry Transmission

This repository contains the ESP32 firmware for the Smart Cushion system. The edge devices are responsible for reading raw values from the Force Sensing Resistors (FSRs), controlling physical alerts, and transmitting telemetry to the Fog Node.

<p align="center">
  <b>ESP32 Microcontrollers ｜ 9x FSR Matrix ｜ Haptic Feedback ｜ UDP Streaming ｜ MQTT Subscriptions</b>
</p>

---

## 🔗 Project Links

| Item                    | Link                                                                                          |
| ----------------------- | --------------------------------------------------------------------------------------------- |
| 🌐 Project Website      | [https://tonguyentanphuong.github.io/smart-cushion-web/](https://github.com/tonguyentanphuong/smart-cushion-web) |
| 📦 Edge Repository      | [https://github.com/MuoiVung/smart-cushion-edge](https://github.com/MuoiVung/smart-cushion-edge) |
| ⚙️ Fog Node Runtime     | [https://github.com/MuoiVung/smart-cushion-fog-node](https://github.com/MuoiVung/smart-cushion-fog-node) |
| ⚙️ Main Architecture    | [https://github.com/MuoiVung/smart-cushion](https://github.com/MuoiVung/smart-cushion)            |

---

## 📌 Component Overview

The Edge Layer acts as the physical interface between the user and the digital system.

Due to the limited number of ADC pins available on an ESP32 when Wi-Fi is active, the system splits the sensing workload across **two ESP32 microcontrollers**.
*   **Primary Node:** Reads 3 sensors, monitors ambient temperature, triggers haptic feedback, and listens for MQTT commands.
*   **Secondary Node:** Reads the remaining 6 sensors and streams data continuously.

---

## 🛠️ Technology Stack

| Layer              | Tools / Components                             |
| ------------------ | ---------------------------------------------- |
| Microcontrollers   | ESP32-S / NodeMCU-32S                          |
| Sensors            | 9x Force Sensing Resistors (FSR), 1x NTC Thermistor |
| Feedback           | 1x Vibration Motor (Coin type)                 |
| Protocols          | Wi-Fi, UDP (fast telemetry), MQTT (commands)   |
| Language / IDE     | C++, Arduino IDE / PlatformIO                  |
| Libraries          | `PubSubClient`, `ArduinoJson`                  |

---

## 💡 Motivation

Proper sensing on the edge is critical to the success of the entire system. Traditional ergonomic seats often use simple pressure switches that only tell if someone is sitting or not. 

Our approach uses a matrix of 9 Force Sensing Resistors (FSRs) to generate a detailed "weight map" of the user's lower body. By offloading the complex classification logic to the Fog Node and only running raw ADC sampling on the ESP32, we achieve:
* 🔋 High-frequency sampling without overheating the MCU.
* ⚡ Ultra-low latency data transmission using connectionless UDP.

---

## 🧩 System Architecture (Edge)

### Hardware Components

| Component | Quantity | Purpose |
|---|---:|---|
| ESP32 Development Board | 2 | Main controllers and Wi-Fi transmission |
| FSR Sensors | 9 | Detect weight distribution in a 3x3 grid |
| 10k Resistors | 9 | Used in voltage dividers with the FSRs |
| NTC Thermistor (100k)| 1 | Ambient temperature sensing |
| Vibration Motor | 1 | Audio/Haptic posture reminder |
| Breadboard & Wires | 1 set | Prototype wiring |

### Data Workflow

| Phase       | Action                                                       |
| ----------- | --------------------------------------------------------------- |
| **1. Sensing** | ADC reads analog voltage from the 9 FSR voltage dividers (0-4095 range). |
| **2. Streaming** | ESP32 packages the 9 values into a UDP datagram and sends them to the Fog Node IP (~10Hz). |
| **3. Listening** | The Primary ESP32 maintains a persistent MQTT subscription to the `cushion/command` topic. |
| **4. Alerting**| If the Fog Node detects prolonged bad posture, it sends a payload. The ESP32 triggers the motor via PWM. |

---

## 🗂️ Repository Structure

```text
smart-cushion-edge/
│
├── README.md
├── esp32_1_firmware/
│   └── esp32_1_firmware.ino    (Controls Left FSRs, Temp, Vibration, MQTT)
├── esp32_2_firmware/
│   └── esp32_2_firmware.ino    (Controls Middle/Right FSRs, UDP Streaming)
└── esp32_secrets.h.template    (Wi-Fi & MQTT Credentials)
```

---

## 🚀 Deployment Guide

### 1. Pre-requisites
1. Install [Arduino IDE](https://www.arduino.cc/en/software).
2. Go to **Tools > Manage Libraries** and install:
    -   `PubSubClient` (for MQTT)
    -   `ArduinoJson`
3. Install the ESP32 Board Support package via the Boards Manager.

### 2. Configuration
1. Rename `esp32_secrets.h.template` to `esp32_secrets.h`.
2. Update the network and IP credentials:
   ```cpp
   #define SECRET_SSID "Your_WiFi_SSID"
   #define SECRET_PASS "Your_WiFi_Password"
   #define FOG_NODE_IP "192.168.x.x" // Local IP of the Fog Node PC
   #define MQTT_SERVER "192.168.x.x" // Local IP of the MQTT Broker
   ```

### 3. Flashing Firmware
1. Connect the **primary ESP32** via USB.
2. Open `esp32_1_firmware.ino`, select your ESP32 Board and COM port, and click **Upload**.
3. Connect the **secondary ESP32** via USB.
4. Open `esp32_2_firmware.ino`, select the COM port, and click **Upload**.

---

## 🎯 Conclusion
The edge layer prioritizes high-frequency sampling and low-latency transmission. By splitting the workload across two microcontrollers and utilizing UDP for raw sensor streaming instead of MQTT or HTTP, the hardware guarantees that the AI engine always has the latest physical state with absolute minimal network overhead.
