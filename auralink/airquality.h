#pragma once
#include <Arduino.h>

void updateAirQualityUI(bool force = false);

class AirQuality {
public:
  AirQuality(size_t window);
  ~AirQuality();

  // NEW: include sensor supply vin (e.g., 3.3 or 5.0). No divider here (direct A0).
  void begin(uint8_t pin, float RLOAD_kOhm, float RZERO_kOhm, float vin_volts);

  // Take one sample and push into the moving average
  void read();

  // Instantaneous AQI from the last sample (no new ADC read)
  float readImmediate() const;

  // Moving average AQI over the window
  float average() const;

  void reset();

private:
  float calculateImmediate(uint16_t raw_adc) const;
  void  add(float v);

  // --- config/state ---
  uint8_t _pin = 36;
  float   _RLOAD = 10.0f;   // kΩ (your 10k pulldown)
  float   _RZERO = 76.63f;  // kΩ (calibrate in clean air: R0 = Rs)
  float   _vin   = 3.3f;    // sensor supply for Rs formula

  // ring buffer
  float*  _buf = nullptr;
  size_t  _window = 0, _idx = 0, _count = 0;
  float   _sum = 0.0f;      // must be float

  // cache last instantaneous
  uint16_t _lastRaw = 0;
  float    _lastImm = NAN;
};
extern AirQuality airQuality;  // moving average over 10 samples