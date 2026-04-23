/**
 * Smart Cushion – ESP32 #2 Firmware
 *
 * Role        : Trục giữa + Cụm Phải (5 FSR sensors)
 * Sensors     : fsr_front_mid, fsr_mid_mid, fsr_back_mid,
 *               fsr_mid_left, fsr_mid_right (Analog ADC 0–4095)
 *
 * MQTT Topics :
 *   Publish   → cushion/raw      (JSON, 0.5 s interval)
 *   (No actuator on ESP32-2; motor is on ESP32-1)
 *
 * Required Libraries:
 *   - PubSubClient  (Nick O'Leary)
 *   - ArduinoJson   (Benoit Blanchon)
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "esp32_secrets.h"

// ── Hardware Pins ──────────────────────────────────────────────────────────
// Pin mapping: 32(FM), 33(MM), 34(BM), 35(ML), 36(MR)
#define PIN_FSR_FM   32   // fsr_front_mid
#define PIN_FSR_MM   33   // fsr_mid_mid   (center)
#define PIN_FSR_BM   34   // fsr_back_mid
#define PIN_FSR_ML   35   // fsr_mid_left
#define PIN_FSR_MR   36   // fsr_mid_right

// ── MQTT ───────────────────────────────────────────────────────────────────
const int    mqtt_port   = 1883;
const char*  topic_raw   = "cushion/raw";

// ── Globals ────────────────────────────────────────────────────────────────
WiFiClient    espClient;
PubSubClient  client(espClient);

unsigned long lastPublish    = 0;
const unsigned long PUB_INTERVAL = 500;   // 0.5 s

// ── Forward declarations ───────────────────────────────────────────────────
void setup_wifi();
void reconnect();

// ── Setup ──────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  pinMode(PIN_FSR_FM, INPUT);
  pinMode(PIN_FSR_MM, INPUT);
  pinMode(PIN_FSR_BM, INPUT);
  pinMode(PIN_FSR_ML, INPUT);
  pinMode(PIN_FSR_MR, INPUT);

  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
  client.setBufferSize(384);
}

// ── WiFi ───────────────────────────────────────────────────────────────────
void setup_wifi() {
  delay(10);
  Serial.printf("\nConnecting to WiFi: %s\n", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\nWiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
}

// ── MQTT Reconnect ─────────────────────────────────────────────────────────
void reconnect() {
  while (!client.connected()) {
    Serial.printf("\nConnecting MQTT to %s ...\n", mqtt_server);
    String clientId = "ESP32-2-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println(">>> FOG NODE CONNECTED <<<");
      // ESP32-2 is send-only; no command subscription needed
    } else {
      Serial.printf("Failed rc=%d – retry in 5 s\n", client.state());
      delay(5000);
    }
  }
}

// ── Main Loop ──────────────────────────────────────────────────────────────
void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  unsigned long now = millis();
  if (now - lastPublish >= PUB_INTERVAL) {
    lastPublish = now;

    int fm = analogRead(PIN_FSR_FM);
    int mm = analogRead(PIN_FSR_MM);
    int bm = analogRead(PIN_FSR_BM);
    int ml = analogRead(PIN_FSR_ML);
    int mr = analogRead(PIN_FSR_MR);

    // Build JSON — matches system_architecture.md Interface 01
    StaticJsonDocument<256> doc;
    doc["device_id"] = "esp32-2";
    doc["timestamp"] = now / 1000.0f;

    JsonObject sensors = doc.createNestedObject("sensors");
    sensors["fsr_front_mid"] = fm;
    sensors["fsr_mid_mid"]   = mm;
    sensors["fsr_back_mid"]  = bm;
    sensors["fsr_mid_left"]  = ml;
    sensors["fsr_mid_right"] = mr;

    char buffer[256];
    serializeJson(doc, buffer);
    client.publish(topic_raw, buffer);

    Serial.printf("[ESP32-2] %s\n", buffer);
  }
}
