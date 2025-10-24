#include "User_Setup.h"
#include "airquality.h"
#include "danger.h"

#include <TFT_eSPI.h>
#include <lvgl.h>
#include <ui.h>
#include <math.h>

AirQuality airQuality(20);

void updateAirQualityUI(bool force) {
    static uint32_t lastUpdate = 0;
    uint32_t now = millis();
    if (now - lastUpdate < 300 && !force) {
        return;
    }
    lastUpdate = now;

    float imm = airQuality.readImmediate();
    float avg = airQuality.average();
    ColorOpacity co = getDangerColorAirQuality(avg);

    lv_label_set_text_fmt(ui_AirQuality, "%.0f", avg);
    lv_obj_set_style_bg_color(ui_AirQualityContainer, co.color, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ui_AirQualityContainer, co.opacity, LV_PART_MAIN);
    Serial.printf("[AIRQUALITY]: imm=%.0f avg=%.0f\n", imm, avg);
}

AirQuality::AirQuality(size_t window) : _window(window) {
  _buf = (_window > 0) ? new float[_window]() : nullptr;
  reset();
}

AirQuality::~AirQuality() {
  delete[] _buf;
  _buf = nullptr;
}

void AirQuality::begin(uint8_t pin, float RLOAD_kOhm, float RZERO_kOhm, float vin_volts) {
  _pin   = pin;
  _RLOAD = RLOAD_kOhm;  // kΩ
  _RZERO = RZERO_kOhm;  // kΩ (set after calibration)
  _vin   = vin_volts;   // e.g., 3.3f if the sensing network runs at 3.3 V

  analogSetPinAttenuation(_pin, ADC_11db); // maximize usable range near 3.3 V
}

void AirQuality::read() {
  // Debounce (your 20 ms), but also ensure we take exactly ONE authoritative sample.
  static uint32_t lastRead = 0;
  uint32_t now = millis();
  if (now - lastRead < 20) return;
  lastRead = now;

  // ESP32 ADC settle: discard first sample from a high-Z node
  (void)analogRead(_pin);
  delayMicroseconds(50);

  uint16_t raw1 = analogRead(_pin);
  uint16_t raw2 = analogRead(_pin);
  uint16_t raw  = (raw1 + raw2) / 2;  // small average for stability

  float imm = calculateImmediate(raw);

  _lastRaw = raw;
  _lastImm = imm;

  add(imm);

  // Debug: one line, single sample
  // Serial.printf("[AIRQUALITY] raw:%u  AQI:%.0f\n", _lastRaw, _lastImm);
}

void AirQuality::reset() {
  _idx = 0;
  _count = 0;
  _sum = 0.0f;
  if (_buf) {
    for (size_t i = 0; i < _window; ++i) _buf[i] = NAN;
  }
}

// Convert a single ADC reading to an AQI score (0..500) via Rs/R0
float AirQuality::calculateImmediate(uint16_t raw_adc) const {
  // ADC (0..4095) -> voltage at A0 (direct, no divider)
  float vAdc = (raw_adc / 4095.0f) * 3.3f;

  // Clamp to avoid divide-by-zero and negative Rs
  float vOut = vAdc;
  if (vOut < 0.01f)         vOut = 0.01f;
  if (vOut > _vin - 0.01f)  vOut = _vin - 0.01f;

  // Rs = (Vin/Vout - 1) * Rload
  float Rs = ((_vin / vOut) - 1.0f) * _RLOAD; // kΩ
  if (Rs < 0.001f) Rs = 0.001f;

  // Ratio vs R0 (≈1.0 in clean air). Lower ratio => worse air.
  float ratio = Rs / _RZERO;

  // Map ratio to a simple AQI-like score (tunable curve)
  const float gamma = 1.6f;   // curvature: higher -> more aggressive rise
  const float base  = 100.0f; // baseline near clean air
  float inv = 1.0f / fmaxf(0.05f, ratio);     // avoid div/0
  float score = base * powf(inv, gamma);

  if (score < 0.0f)   score = 0.0f;
  if (score > 500.0f) score = 500.0f;

  return score;
}

float AirQuality::readImmediate() const {
  return _lastImm; // return last computed instantaneous AQI (no new ADC read)
}

float AirQuality::average() const {
  return (_count > 0) ? (_sum / (float)_count) : NAN;
}

void AirQuality::add(float v) {
  if (!_buf || _window == 0) return;

  if (_count < _window) {
    _sum += v;
    _buf[_idx++] = v;
    _count++;
    if (_idx == _window) _idx = 0;
  } else {
    _sum -= _buf[_idx];
    _sum += v;
    _buf[_idx++] = v;
    if (_idx == _window) _idx = 0;
  }
}
