// define threshold for each sensor reading and return a color code
// Illuminatioon levels (in lux)
// Air Quality Index (AQI) levels
// Relative Humidity levels (in %)
// Temperature levels (in °C)
// Pressure levels (in hPa)
// Loudness levels (in dB)

#pragma once
#include <Arduino.h>
#include <lvgl.h>

struct ColorOpacity {
  lv_color_t color;
  uint8_t opacity;  // 0-255
};

ColorOpacity getDangerColorAirQuality(float aqi);
ColorOpacity getDangerColorIllumination(float lux);
ColorOpacity getDangerColorHumidity(float rh);
ColorOpacity getDangerColorTemperature(float celsius);
ColorOpacity getDangerColorPressure(float hpa);
ColorOpacity getDangerColorLoudness(float db);