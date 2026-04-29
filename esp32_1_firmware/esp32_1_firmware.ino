/**
 * Smart Cushion – ESP32 #1 Firmware
 *
 * Role        : Cụm Trái + Cảm biến Nhiệt độ + Actuator rung
 * Sensors     : fsr_front_left, fsr_front_right, fsr_back_left,
 *               fsr_back_right (Analog ADC 0–4095) + NTC thermistor
 * Actuator    : Vibration motor (PWM on PIN_MOTOR)
 *
 * MQTT Topics :
 *   Publish   → cushion/raw      (JSON, 0.5 s interval)
 *   Subscribe ← cushion/command  (JSON command from Fog)
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
#define PIN_FSR_FL   32   // fsr_front_left
#define PIN_FSR_FR   33   // fsr_front_right
#define PIN_FSR_BL   34   // fsr_back_left
#define PIN_FSR_BR   35   // fsr_back_right
#define PIN_NTC      36   // NTC thermistor → temperature
#define PIN_MOTOR    25   // PWM motor output (LEDC channel 0)

// ── LEDC (PWM) config ──────────────────────────────────────────────────────
#define LEDC_CHANNEL   0
#define LEDC_FREQ_HZ   5000
#define LEDC_RESOLUTION 8   // 8-bit → 0–255

// ── MQTT ───────────────────────────────────────────────────────────────────
const int    mqtt_port       = 1883;
const char*  topic_raw       = "cushion/raw";
const char*  topic_command   = "cushion/command";

// ── Globals ────────────────────────────────────────────────────────────────
WiFiClient    espClient;
PubSubClient  client(espClient);

unsigned long lastPublish     = 0;
const unsigned long PUB_INTERVAL = 500;   // 0.5 s

bool          vibration_active = false;   // current motor state

// ── Forward declarations ───────────────────────────────────────────────────
void setup_wifi();
void reconnect();
void callback(char* topic, byte* payload, unsigned int length);
float convertNTCToTemperature(int analogValue);
void  setMotor(bool active, int intensity = 0);
void  runPattern(const char* pattern, int intensity);

// ── Setup ──────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  // FSR inputs
  pinMode(PIN_FSR_FL, INPUT);
  pinMode(PIN_FSR_FR, INPUT);
  pinMode(PIN_FSR_BL, INPUT);
  pinMode(PIN_FSR_BR, INPUT);
  pinMode(PIN_NTC,    INPUT);

  // Motor PWM via LEDC (ESP32 Core 3.x API)
  ledcAttach(PIN_MOTOR, LEDC_FREQ_HZ, LEDC_RESOLUTION);
  ledcWrite(PIN_MOTOR, 0);   // Motor off at boot

  setup_wifi();

  // Initial server setup (will be updated in reconnect if needed)
  client.setServer(mqtt_servers[0], mqtt_port);
  client.setCallback(callback);
  client.setBufferSize(512);
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

// ── MQTT Command Callback ──────────────────────────────────────────────────
/**
 * Fog → Edge command schema (cushion/command):
 *   { "device_id": "esp32-1", "command": "vibrate",
 *     "active": true, "pattern": "short_triple", "intensity": 255 }
 *   { "device_id": "esp32-1", "command": "vibrate", "active": false }
 */
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("\n[MQTT CMD] Topic: %s\n", topic);

  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, payload, length);
  if (err) {
    Serial.printf("JSON parse error: %s\n", err.f_str());
    return;
  }

  // Filter by device_id
  const char* target = doc["device_id"] | "";
  if (strlen(target) > 0 && strcmp(target, "esp32-1") != 0) return;

  const char* command = doc["command"] | "";
  if (strcmp(command, "vibrate") != 0) return;

  bool   active    = doc["active"]    | false;
  int    intensity = doc["intensity"] | 200;
  const char* pattern = doc["pattern"] | "short_triple";

  if (active) {
    runPattern(pattern, intensity);
    vibration_active = true;
  } else {
    setMotor(false);
    vibration_active = false;
  }
}

// ── Motor helpers ──────────────────────────────────────────────────────────
void setMotor(bool active, int intensity) {
  ledcWrite(PIN_MOTOR, active ? constrain(intensity, 0, 255) : 0);
}

void runPattern(const char* pattern, int intensity) {
  intensity = constrain(intensity, 0, 255);

  if (strcmp(pattern, "short_triple") == 0) {
    for (int i = 0; i < 3; i++) {
      setMotor(true, intensity); delay(150);
      setMotor(false);          delay(100);
    }
  } else if (strcmp(pattern, "long_single") == 0) {
    setMotor(true, intensity); delay(800);
    setMotor(false);
  } else {
    // Default: single short buzz
    setMotor(true, intensity); delay(300);
    setMotor(false);
  }
}

// ── MQTT Reconnect ─────────────────────────────────────────────────────────
void reconnect() {
  while (!client.connected()) {
    for (int i = 0; i < num_servers; i++) {
      const char* current_server = mqtt_servers[i];
      Serial.printf("\nTrying Fog Node at %s ...\n", current_server);
      client.setServer(current_server, mqtt_port);

      String clientId = "ESP32-1-" + String(random(0xffff), HEX);
      if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
        Serial.printf(">>> CONNECTED TO FOG NODE AT %s <<<\n", current_server);
        client.subscribe(topic_command);
        Serial.printf("Subscribed: %s\n", topic_command);
        return; // Success!
      } else {
        Serial.printf("Failed (rc=%d)\n", client.state());
      }
      
      if (i < num_servers - 1) {
        delay(1000); // Wait a bit before trying the next IP
      }
    }
    
    Serial.println("Could not connect to any Fog Node. Retrying in 5s...");
    delay(5000);
  }
}

// ── NTC → Temperature ──────────────────────────────────────────────────────
float convertNTCToTemperature(int rawAdc) {
  if (rawAdc <= 0 || rawAdc >= 4095) return 25.0f;
  
  // Sơ đồ: VCC --- [NTC 100k] --- [Chân 36] --- [Trở 10k] --- GND
  float resistance = 10000.0f * (4095.0f / (float)rawAdc - 1.0f);
  
  // NTC của bạn là loại 100k (100,000 Ohm ở 25 độ C)
  float steinhart = log(resistance / 100000.0f) / 3950.0f;
  steinhart += 1.0f / (25.0f + 273.15f);
  return (1.0f / steinhart) - 273.15f;
}

// ── Main Loop ──────────────────────────────────────────────────────────────
void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  unsigned long now = millis();
  if (now - lastPublish >= PUB_INTERVAL) {
    lastPublish = now;

    int   fl   = analogRead(PIN_FSR_FL);
    int   fr   = analogRead(PIN_FSR_FR);
    int   bl   = analogRead(PIN_FSR_BL);
    int   br   = analogRead(PIN_FSR_BR);
    float temp = convertNTCToTemperature(analogRead(PIN_NTC));

    // Build JSON — matches system_architecture.md Interface 01
    StaticJsonDocument<384> doc;
    doc["device_id"] = "esp32-1";
    doc["timestamp"] = now / 1000.0f;

    JsonObject sensors = doc.createNestedObject("sensors");
    sensors["fsr_front_left"]  = fl;
    sensors["fsr_front_right"] = fr;
    sensors["fsr_back_left"]   = bl;
    sensors["fsr_back_right"]  = br;
    sensors["temperature"]     = round(temp * 10.0f) / 10.0f;

    JsonObject actuator = doc.createNestedObject("actuator");
    actuator["vibration_status"] = vibration_active;

    char buffer[384];
    serializeJson(doc, buffer);
    client.publish(topic_raw, buffer);

    Serial.printf("[ESP32-1] %s\n", buffer);
  }
}
