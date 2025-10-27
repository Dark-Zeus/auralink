#include "thermohygrometer.h"
#include "User_Setup.h"
#include "danger.h"
#include "lv_functions.h"

#include <TFT_eSPI.h>
#include <lvgl.h>
#include <ui.h>


Thermohygrometer thermohygrometer(20);  // DHT22 on pin 42

void updateThermohygrometerUI(bool force) {
    // One-time: watch these globals so they auto-NULL on LV_EVENT_DELETE
    static bool _hooked = false;
    if (!_hooked) {
        LV_WATCH_DELETE(ui_TemperatureContainer, &ui_TemperatureContainer);
        LV_WATCH_DELETE(ui_Temperature, &ui_Temperature);
        LV_WATCH_DELETE(ui_RelativeHumidityContainer, &ui_RelativeHumidityContainer);
        LV_WATCH_DELETE(ui_RelativeHumidity, &ui_RelativeHumidity);
        _hooked = true;
    }

    static uint32_t lastUpdate = 0;
    uint32_t now = millis();
    if (now - lastUpdate < UI_SENSOR_UPDATE_INTERVAL_MS && !force) return;
    lastUpdate = now;

    static float lastAvgTemp = NAN;
    static float lastAvgHum = NAN;

    TemperatureHumidity avg = thermohygrometer.average();
    float avgTemp = avg.temperature;
    float avgHum = avg.humidity;

    // Temperature block (guarded)
    if (force || isnan(lastAvgTemp) || fabsf(avgTemp - lastAvgTemp) >= 0.1f) {
        ColorOpacity co = getDangerColorTemperature(avgTemp);

        LV_SAFE(ui_TemperatureContainer, do {
            lv_obj_set_style_bg_color(ui_TemperatureContainer, co.color, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa  (ui_TemperatureContainer, co.opacity, LV_PART_MAIN | LV_STATE_DEFAULT); } while (0));

        LV_SAFE(ui_Temperature, lv_label_set_text_fmt(ui_Temperature, "%.1f", avgTemp));

        lastAvgTemp = avgTemp;
    }

    // Humidity block (guarded)
    if (force || isnan(lastAvgHum) || fabsf(avgHum - lastAvgHum) >= 0.1f) {
        ColorOpacity co = getDangerColorHumidity(avgHum);

        LV_SAFE(ui_RelativeHumidityContainer, do {
            lv_obj_set_style_bg_color(ui_RelativeHumidityContainer, co.color, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa  (ui_RelativeHumidityContainer, co.opacity, LV_PART_MAIN | LV_STATE_DEFAULT); } while (0));

        LV_SAFE(ui_RelativeHumidity, lv_label_set_text_fmt(ui_RelativeHumidity, "%.1f", avgHum));

        lastAvgHum = avgHum;
    }

    Serial.printf("[THERMOHYGROMETER]: temp_imm=%.2f temp_avg=%.2f hum_imm=%.2f hum_avg=%.2f\n",
                  thermohygrometer.readImmediateTemperature(), avgTemp,
                  thermohygrometer.readImmediateHumidity(), avgHum);
}

Thermohygrometer::Thermohygrometer(size_t window) : _window(window) {
    _buf = (_window > 0)
               ? new TemperatureHumidity[_window]
               : nullptr;  // CHANGED: TemperatureHumidity buffer, zero-init
    reset();
}

Thermohygrometer::~Thermohygrometer() {
    delete[] _buf;
    _buf = nullptr;
}

bool Thermohygrometer::begin(uint8_t pin, uint8_t type) {
    _pin = pin;
    _type = type;
    _dht = new DHT(pin, type);
    _dht->begin();
    delay(2000);  // allow sensor to stabilize
    return true;
}

void Thermohygrometer::read() {
    TemperatureHumidity th = readImmediate();
    if (!isnan(th.temperature) && !isnan(th.humidity)) {
        add(th);
    }
}

void Thermohygrometer::readTemperature() {
    float t = _dht->readTemperature();
    if (!isnan(t)) {
        TemperatureHumidity th;
        th.temperature = t;
        th.humidity = NAN;
        add(th);
    }
}

void Thermohygrometer::readHumidity() {
    float h = _dht->readHumidity();
    if (!isnan(h)) {
        TemperatureHumidity th;
        th.temperature = NAN;
        th.humidity = h;
        add(th);
    }
}

void Thermohygrometer::reset() {
    _idx = 0;
    _count = 0;
    _sumTemp = 0;
    _sumHum = 0;
    if (_buf) {
        for (size_t i = 0; i < _window; ++i) {
            _buf[i].temperature = NAN;
            _buf[i].humidity = NAN;
        }
    }
}

TemperatureHumidity Thermohygrometer::readImmediate() const {
    TemperatureHumidity th;
    th.temperature = _dht->readTemperature();
    th.humidity = _dht->readHumidity();
    return th;
}

float Thermohygrometer::readImmediateTemperature() const {
    return _dht->readTemperature();
}

float Thermohygrometer::readImmediateHumidity() const {
    return _dht->readHumidity();
}

TemperatureHumidity Thermohygrometer::average() const {
    TemperatureHumidity th;
    th.temperature = (_count > 0) ? static_cast<float>(_sumTemp) / _count : NAN;
    th.humidity = (_count > 0) ? static_cast<float>(_sumHum) / _count : NAN;
    return th;
}

void Thermohygrometer::add(TemperatureHumidity v) {
    // Remove oldest value from sums
    if (_count == _window) {
        if (!isnan(_buf[_idx].temperature)) {
            _sumTemp -= _buf[_idx].temperature;
        }
        if (!isnan(_buf[_idx].humidity)) {
            _sumHum -= _buf[_idx].humidity;
        }
    } else {
        ++_count;
    }

    // Add new value to buffer and sums
    _buf[_idx] = v;
    if (!isnan(v.temperature)) {
        _sumTemp += v.temperature;
    }
    if (!isnan(v.humidity)) {
        _sumHum += v.humidity;
    }

    // Update index
    _idx = (_idx + 1) % _window;
}