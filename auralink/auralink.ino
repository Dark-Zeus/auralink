#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <lvgl.h>
#include <ui.h>

#include "User_Setup.h"
#include "airquality.h"
#include "battery.h"
#include "display.h"
#include "display_manager.h"
#include "illumination.h"
#include "mqtt.h"
#include "pressure.h"
#include "thermohygrometer.h"
//#include "thb.h"
#include "uv.h"
#include "time_source.h"
#include "update_ui.h"
#include "wifi_connector.h"

Display display;

DisplayManager::Params dmParams;
DisplayManager displayManager(TOUCH_LEFT_PIN, TOUCH_RIGHT_PIN, dmParams);

extern lv_obj_t* ui_SensorData;
extern lv_obj_t* ui_DailyQuote;
extern lv_obj_t* ui_EmailSummary;
static lv_obj_t* gScreens[3];
static const uint8_t gScreenCount = 3;

WifiConnector::Params wifiParams;
WifiConnector wifi(wifiParams);
WiFiClient net;

MqttClient mqtt;

UV uvSensor(20);

void onMqttMessage(const String& topic, const uint8_t* payload, size_t len) {
  updateMqttUI(false, mqtt.connected(), true, false);
  Serial.printf("[MQTT] %s => %.*s\n", topic.c_str(), (int)len, (const char*)payload);

  // Payload is json
  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, payload, len);
  if (error) {
    Serial.printf("[MQTT] JSON parse error: %s\n", error.c_str());
    return;
  }

  if (topic == MQTT_TOPIC_COMMAND) {
    // Handle command
  } else if (topic == MQTT_TOPIC_EMAIL_SUMMARY) {
    const char* summary = doc["summary"] | "";
    Serial.printf("[MQTT] Email Summary: %s\n", summary);
    updateEmailSummary(summary);
  } else if (topic == MQTT_TOPIC_DAILY_QUOTE) {
    const char* quote = doc["quote"] | "";
    Serial.printf("[MQTT] Daily Quote: %s\n", quote);
    updateDailyQuote(quote);
  } else if (topic == MQTT_TOPIC_PREDICTION) {
    const char* prediction = doc["prediction"] | "";
    Serial.printf("[MQTT] Prediction: %s\n", prediction);
    // Handle prediction
  } else if (topic == MQTT_TOPIC_ALERT) {
    const char* alert = doc["alert"] | "";
    Serial.printf("[MQTT] Alert: %s\n", alert);
    // Handle alert
  } else {
    Serial.printf("[MQTT] Unhandled topic: %s\n", topic.c_str());
  }
}

void onMqttPublish(const String& topic, const uint8_t* payload, size_t len, bool retain) {
  if (len == 0) {
    mqtt.publish(MQTT_TOPIC_STATUS, payload, retain);
  } else {
    mqtt.publish(MQTT_TOPIC_SENSOR, payload, len, retain);
  }
  updateMqttUI(false, mqtt.connected(), false, true);
}

void subscribeMqttTopics() {
  // Subscriptions are now handled in setup after connecting
  if (mqtt.subscribe(MQTT_TOPIC_COMMAND, 0)) {
    Serial.println("[MQTT] Subscribed to command topic.");
  } else {
    Serial.println("[MQTT] Failed to subscribe to command topic.");
  }
  if (mqtt.subscribe(MQTT_TOPIC_EMAIL_SUMMARY, 0)) {
    Serial.println("[MQTT] Subscribed to email summary topic.");
  } else {
    Serial.println("[MQTT] Failed to subscribe to email summary topic.");
  }

  if (mqtt.subscribe(MQTT_TOPIC_DAILY_QUOTE, 0)) {
    Serial.println("[MQTT] Subscribed to daily quote topic.");
  } else {
    Serial.println("[MQTT] Failed to subscribe to daily quote topic.");
  }

  if (mqtt.subscribe(MQTT_TOPIC_PREDICTION, 0)) {
    Serial.println("[MQTT] Subscribed to prediction topic.");
  } else {
    Serial.println("[MQTT] Failed to subscribe to prediction topic.");
  }
  if (mqtt.subscribe(MQTT_TOPIC_ALERT, 0)) {
    Serial.println("[MQTT] Subscribed to alert topic.");
  } else {
    Serial.println("[MQTT] Failed to subscribe to alert topic.");
  }
}

void setup() {
  Serial.begin(115200); /* prepare for possible serial debug */

  String LVGL_Arduino = "[LVGL] Hello Arduino! ";
  LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

  Serial.println(LVGL_Arduino);
  Serial.println("[LVGL] I am LVGL_Arduino");

  if (!display.begin(SCREEN_W, SCREEN_H, /*rotation*/ 2)) {
    Serial.println("[DISPLAY] init failed");
    while (true) delay(1000);
  }

  ui_init();

  gScreens[0] = ui_SensorData;
  gScreens[1] = ui_DailyQuote;
  gScreens[2] = ui_EmailSummary;
  displayManager.begin(gScreens, gScreenCount);

  wifi.begin(WIFI_SSID, WIFI_PASS);
  updateWifiUI(true, wifi.isConnected(), wifi.rssi());
  // if (wifi.waitForConnect(15000)) {
  //   Serial.println("[WIFI] Connected successfully.");
  // } else {
  //   Serial.println("[WIFI] Connection timed out.");
  // }

  MqttClient::Params mp;
  mp.clientId = MQTT_CLIENT_ID;
  mp.username = MQTT_USERNAME;
  mp.password = MQTT_PASSWORD;
  mp.willTopic = MQTT_TOPIC_WILL;
  mp.willPayload = MQTT_TOPIC_WILL_PAYLOAD_OFFLINE;
  mp.willQos = 0;
  mp.willRetain = true;
  mp.cleanSession = true;
  mp.keepAliveSec = 15;

  mqtt = MqttClient(mp);
  mqtt.begin(net, MQTT_HOST, MQTT_PORT);
  updateMqttUI(true, mqtt.connected(), false, false);
  mqtt.onMessage(onMqttMessage);

  if (mqtt.connectNow()) {
    Serial.println("[MQTT] Connected to broker.");
  } else {
    Serial.println("[MQTT] Connection to broker failed.");
  }

  mqtt.setOnlineMessage(MQTT_TOPIC_WILL, MQTT_TOPIC_STATUS_PAYLOAD_ONLINE, 0, true);
  subscribeMqttTopics();

  analogReadResolution(12);                              // 0..4095
  analogSetPinAttenuation(BATTERY_LEVEL_PIN, ADC_11db);  // up to ~3.3V full-scale

  pinMode(BATTERY_CHARGER_PIN, INPUT_PULLDOWN);  // or INPUT_PULLUP if wired that way
  chargerLevel = digitalRead(BATTERY_CHARGER_PIN);
  manageChargingState();
  attachInterrupt(digitalPinToInterrupt(BATTERY_CHARGER_PIN), charger_isr, CHANGE);
  battery.read();
  updateBatteryUI(true);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(100000);
  Wire.setTimeout(20);

  timeSource.begin(&Wire, GMT_OFFSET_SEC, NTP_SERVER);
  if (wifi.isConnected()) {
    Serial.println("[TIMESOURCE] Network connected; syncing time from NTP.");
    timeSource.syncCurrentTime();
  } else {
    Serial.println("[TIMESOURCE] Network not connected; using RTC time.");
  }
  updateTimeSourceUI(true);

  if (!illuminationMeter.begin(ILLUMINATION_SENSOR_ADDRESS, &Wire)) {
    Serial.println("[ILLUMINATION] init failed — check wiring/address.");
  }

  if (!thermohygrometer.begin(DHT_PIN, DHT_TYPE)) {
    Serial.println("[THERMOHYGROMETER] init failed — check wiring/type.");
  }

  airQuality.begin(MQ135_PIN, 10.0, 76.63, 5.0);  // MQ135 on pin 8

  if (!pressureSensor.begin()) {
    Serial.println("[PRESSURE] init failed — check wiring/address.");
  }

  // if (!thbSensor.begin(THB_SENSOR_ADDRESS, &Wire)) {
  //   Serial.println("[THB] init failed — check wiring/address.");
  // }

  uvSensor.begin(UV_SENSOR_PIN);


  Serial.println("[SYSTEM] Setup done");
}

void loop() {
  displayManager.loop();

  if (wifi.isConnected()) {
    timeSource.syncCurrentTime();
  }
  updateTimeSourceUI(false);

  wifi.loop();
  updateWifiUI(false, wifi.isConnected(), wifi.rssi());
  mqtt.loop();
  updateMqttUI(false, mqtt.connected(), false, false);

  if (chargerEvent) {
    manageChargingState();
  }

  battery.read();
  updateBatteryUI(false);

  illuminationMeter.read();
  updateIlluminationUI(false);

  airQuality.read();
  updateAirQualityUI(false);

  pressureSensor.read();
  updatePressureUI(false);

  thermohygrometer.read();
  updateThermohygrometerUI(false);

  // thbSensor.read();
  // updateTHBUI(false);

  uvSensor.read();
  updateUVIndexUI(uvSensor, false);

  static uint32_t last = 0;
  if (millis() - last > SENSOR_PUBLISH_INTERVAL_MS && mqtt.connected()) {
    last = millis();

    StaticJsonDocument<256> doc;
    doc["battery_percent"] = battery.percent();
    doc["illumination_lux"] = illuminationMeter.average();
    doc["temperature_c"] = pressureSensor.average();
    doc["humidity_percent"] = thermohygrometer.average().humidity;
    doc["air_quality_aqi"] = airQuality.average();
    doc["pressure_pa"] = thermohygrometer.average().pressure;

    char buf[256];
    size_t n = serializeJson(doc, buf, sizeof(buf));

    onMqttPublish(MQTT_TOPIC_SENSOR, (const uint8_t*)buf, n, /*retain=*/false);
  }

  display.loop();
  delay(5);
}