#pragma once
#include <Arduino.h>
#include <DHT.h>

#define UI_SENSOR_UPDATE_INTERVAL_MS 2000
#define SENSOR_PUBLISH_INTERVAL_MS 10000

#define I2C_SDA_PIN 17
#define I2C_SCL_PIN 18

#define BATTERY_CHARGER_PIN 2
#define BATTERY_LEVEL_PIN 7

#define DHT_PIN 42
#define DHT_TYPE DHT22

#define ILLUMINATION_SENSOR_ADDRESS 0x5C

#define SCREEN_W 128
#define SCREEN_H 160

#define TOUCH_LEFT_PIN 35
#define TOUCH_RIGHT_PIN 36

#define WIFI_SSID "DarkZeus4G"
#define WIFI_PASS "dzeus2002"

#define MQTT_HOST "10.0.0.79"
#define MQTT_PORT 1883

#define MQTT_CLIENT_ID "AuraLink-Node"
#define MQTT_USERNAME "admin"
#define MQTT_PASSWORD "admin"

#define MQTT_TOPIC_WILL "auralink/status"
#define MQTT_TOPIC_WILL_PAYLOAD_OFFLINE "{\"status\":\"offline\"}"
#define MQTT_TOPIC_STATUS_PAYLOAD_ONLINE "{\"status\":\"online\"}"

#define MQTT_TOPIC_SENSOR "auralink/sensor"
#define MQTT_TOPIC_COMMAND "auralink/command/#"
#define MQTT_TOPIC_STATUS "auralink/status"
#define MQTT_TOPIC_EMAIL_SUMMARY "auralink/email"
#define MQTT_TOPIC_DAILY_QUOTE "auralink/dailyquote"
#define MQTT_TOPIC_PREDICTION "auralink/prediction"
#define MQTT_TOPIC_ALERT "auralink/alert"

// ==== Local User_Setup.h (in your sketch folder) ====
#define USER_SETUP_LOADED   // tell TFT_eSPI we provide everything here

// --- Display driver & size ---
#define ST7735_DRIVER
#define TFT_WIDTH  128
#define TFT_HEIGHT 160

// Your module: green tab on ribbon, red PCB
#define ST7735_BLACKTAB    // try GREENTAB2 first
#define TFT_RGB_ORDER TFT_RGB   // fix common color swap on green-tab boards

// --- Pin map (match your working Adafruit sketch) ---
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   6
#define TFT_DC   5
#define TFT_RST  4
// Backlight pin (set HIGH in your sketch)
#define TFT_BL    2

// --- SPI speed & options ---
#define SPI_FREQUENCY  27000000    // start safe; raise to 40 MHz after it works
#define SPI_READ_FREQUENCY 20000000
#define SPI_TOUCH_FREQUENCY 2500000

#define USE_HSPI_PORT

// No touch/SD on this bus:
#undef TOUCH_CS
#undef SD_CS
