#include "User_Setup.h"
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <ui.h>

#include "display.h"
#include "display_manager.h"
#include "battery.h"
#include "illumination.h"
#include "thermohygrometer.h"
#include "airquality.h"
#include "pressure.h"
#include "wifi_connector.h"

constexpr uint8_t I2C_SDA_PIN = 17;
constexpr uint8_t I2C_SCL_PIN = 18;

constexpr uint8_t DHT_PIN = 42;
constexpr uint8_t DHT_TYPE = DHT22;

constexpr uint16_t ILLUMINATION_SENSOR_ADDRESS = 0x5C;
/*Don't forget to set Sketchbook location in File/Preferences to the path of your UI project (the parent foder of this INO file)*/

/*Change to your screen resolution*/
Display display;
static const uint16_t SCREEN_W = 128;
static const uint16_t SCREEN_H = 160;

constexpr uint8_t TOUCH_LEFT_PIN = 35;
constexpr uint8_t TOUCH_RIGHT_PIN = 36;

DisplayManager::Params dmParams; // tweak if needed
DisplayManager displayManager(TOUCH_LEFT_PIN, TOUCH_RIGHT_PIN, dmParams);

extern lv_obj_t* ui_SensorData;
extern lv_obj_t* ui_DailyQuote;
extern lv_obj_t* ui_EmailSummary;
static lv_obj_t* gScreens[3];
static const uint8_t gScreenCount = 3;  

WifiConnector::Params wifiParams; // tweak if needed
WifiConnector wifi(wifiParams);

void setup() {
  Serial.begin(115200); /* prepare for possible serial debug */

  String LVGL_Arduino = "[LVGL] Hello Arduino! ";
  LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

  Serial.println(LVGL_Arduino);
  Serial.println("[LVGL] I am LVGL_Arduino");

  if (!display.begin(SCREEN_W, SCREEN_H, /*rotation*/2)) {
    Serial.println("[DISPLAY] init failed");
    while (true) delay(1000);
  }

  ui_init();

  gScreens[0] = ui_SensorData;
  gScreens[1] = ui_DailyQuote;
  gScreens[2] = ui_EmailSummary;
  displayManager.begin(gScreens, gScreenCount);

  wifi.begin("DarkZeus4G", "dzeus2002");
  if (wifi.waitForConnect(15000)) {
    Serial.println("[WIFI] Connected successfully.");
  } else {
    Serial.println("[WIFI] Connection timed out.");
  }

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

  if (!illuminationMeter.begin(ILLUMINATION_SENSOR_ADDRESS, &Wire)) {
    Serial.println("[ILLUMINATION] init failed — check wiring/address.");
  }

  if (!thermohygrometer.begin(DHT_PIN, DHT_TYPE)) {
    Serial.println("[THERMOHYGROMETER] init failed — check wiring/type.");
  }

  airQuality.begin(8, 10.0, 76.63, 5.0);  // MQ135 on pin 8

  if(!pressureSensor.begin()) {
    Serial.println("[PRESSURE] init failed — check wiring.");
  }

  Serial.println("[SYSTEM] Setup done");

}

void loop() {
  
  displayManager.loop();
  wifi.loop(); 

  if (chargerEvent) {
    manageChargingState();
  }

  battery.read();
  updateBatteryUI(false);

  illuminationMeter.read();
  updateIlluminationUI(false);
  
  thermohygrometer.read();
  updateThermohygrometerUI(false);

  airQuality.read();
  updateAirQualityUI(false);

  pressureSensor.read();
  updatePressureUI(false);

  display.loop();
  delay(5);
}