#include <WiFi.h>
#include <PubSubClient.h>

// ---------- WiFi ----------
const char* ssid     = "DarkZeus4G";
const char* password = "dzeus2002";

// ---------- MQTT (Mosquitto on your PC) ----------
const char* mqtt_server = "10.0.0.79";   // your PC's LAN IP
const int   mqtt_port   = 1883;
const char* mqtt_user   = "admin";
const char* mqtt_pass   = "admin";       // set to your real password

// ---------- Topics ----------
const char* TOPIC_STATUS   = "auralink/status";
const char* TOPIC_SENSOR   = "auralink/sensor";
const char* TOPIC_CMD_WILD = "auralink/cmd/#";
const char* TOPIC_LED_CMD  = "auralink/cmd/led";

// ---------- Hardware ----------
const int LED_PIN = 2; // builtin LED on many ESP32 dev kits

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
unsigned long lastMsg = 0;

void setup_wifi() {
  Serial.printf("\nConnecting to %s", ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }
  Serial.printf("\nWiFi OK, IP: %s\n", WiFi.localIP().toString().c_str());
}

void handle_mqtt_messages(char* topic, byte* payload, unsigned int len) {
  // Copy payload to a C string
  static char msg[256];
  len = min(len, (unsigned int)sizeof(msg) - 1);
  memcpy(msg, payload, len);
  msg[len] = '\0';

  Serial.printf("[RX] topic=%s payload=%s\n", topic, msg);

  if (strcmp(topic, TOPIC_LED_CMD) == 0) {
    if (strcasecmp(msg, "ON") == 0) {
      digitalWrite(LED_PIN, HIGH);
      Serial.printf("LED ON\n");
      mqtt.publish(TOPIC_STATUS, "LED turned ON");
    } else if (strcasecmp(msg, "OFF") == 0) {
      digitalWrite(LED_PIN, LOW);
      Serial.printf("LED OFF\n");
      mqtt.publish(TOPIC_STATUS, "LED turned OFF");
    } else {
      Serial.printf("LED UNK\n");
      mqtt.publish(TOPIC_STATUS, "Unknown LED command");
    }
  } else {
    // Echo any other command to status
    mqtt.publish(TOPIC_STATUS, msg);
  }
}

void reconnect_mqtt() {
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Make a unique client ID if you flash multiple devices
    String cid = String("ESP32Client-") + String((uint32_t)ESP.getEfuseMac(), HEX);
    const char* willTopic   = TOPIC_STATUS;
    const char* willPayload = "ESP32 offline";

    if (mqtt.connect(cid.c_str(), mqtt_user, mqtt_pass, willTopic, 1, true, willPayload)) {
      Serial.println("connected");
      mqtt.publish(TOPIC_STATUS, "ESP32 online");
      mqtt.subscribe(TOPIC_CMD_WILD); // auralink/cmd/#
    } else {
      Serial.printf("failed, rc=%d; retry in 5s\n", mqtt.state());
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  setup_wifi();
  mqtt.setServer(mqtt_server, mqtt_port);
  mqtt.setCallback(handle_mqtt_messages);

  // Optional: bigger MQTT buffer if you plan larger payloads
  // mqtt.setBufferSize(512);

  randomSeed(esp_random());
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    setup_wifi();
  }
  if (!mqtt.connected()) {
    reconnect_mqtt();
  }
  mqtt.loop(); // process incoming messages

  // Periodic publish
  unsigned long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;

    float temperature = 25.0 + random(-100, 100) / 10.0;
    float humidity    = 60.0 + random(-200, 200) / 10.0;

    char payload[128];
    snprintf(payload, sizeof(payload),
             "{\"temperature\": %.2f, \"humidity\": %.2f}", temperature, humidity);

    Serial.printf("[TX] %s\n", payload);
    mqtt.publish(TOPIC_SENSOR, payload, true); // retained = true
  }
}
