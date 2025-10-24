#include "User_Setup.h"
#include "pressure.h"
#include "danger.h"

#include "TFT_eSPI.h"
#include <lvgl.h>
#include <ui.h>

Pressure pressureSensor(20);

void updatePressureUI(bool force) {
    static uint32_t lastUpdate = 0;
    uint32_t now = millis();
    // 30s debounce
    if (now - lastUpdate < UI_SENSOR_UPDATE_INTERVAL_MS && !force) {
        return;
    }
    lastUpdate = now;

    float pres_imm = pressureSensor.readImmediatePressure();
    float temp_imm = pressureSensor.readImmediateTemperature();
    float pres_avg = pressureSensor.averagePressure();
    float temp_avg = pressureSensor.averageTemperature();

    ColorOpacity co = getDangerColorPressure(pres_avg);
    lv_obj_set_style_bg_color(ui_PressureContainer, co.color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_PressureContainer, co.opacity, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text_fmt(ui_Pressure, "%.2f", pres_avg);
    Serial.printf("[PRESSURE]: pres_imm=%.2f pres_avg=%.2f temp_imm=%.2f temp_avg=%.2f\n",
                  pres_imm, pres_avg, temp_imm, temp_avg);
}

Pressure::Pressure(size_t window) : _bmp(), _window(window) {
    _buf = (_window > 0) ? new PressureTemperature[_window]() : nullptr;
    reset();
}

Pressure::~Pressure() {
    delete[] _buf;
    _buf = nullptr;
}

bool Pressure::begin() {
    if (!_bmp.begin()) {
        Serial.println("[BMP180] Could not find a valid BMP180 sensor, check wiring!");
        return false;
    }
    _ok = true;
    return true;
}

void Pressure::read() {
    if (!_ok) {
        Serial.println("[BMP180] Sensor not initialized!");
        return;
    }

    PressureTemperature pt;
    pt.temperature = _bmp.readTemperature();
    pt.pressure = _bmp.readPressure() / 100.0f;  // convert to hPa

    add(pt);
}

void Pressure::reset() {
    _idx = 0;
    _count = 0;
    _sumPressure = 0.0f;
    _sumTemperature = 0.0f;
    if (_buf && _window) {
        for (size_t i = 0; i < _window; ++i) {
            _buf[i].pressure = 0.0f;
            _buf[i].temperature = 0.0f;
        }
    }
}

PressureTemperature Pressure::readImmediate() {
    PressureTemperature pt;
    pt.temperature = _bmp.readTemperature();
    pt.pressure = _bmp.readPressure() / 100.0f;  // convert to hPa
    return pt;
}

float Pressure::readImmediatePressure() {
    return _bmp.readPressure() / 100.0f;  // convert to hPa
}

float Pressure::readImmediateTemperature() {
    return _bmp.readTemperature();
}

PressureTemperature Pressure::average() const {
    PressureTemperature pt;
    if (_count == 0) {
        pt.pressure = 0.0f;
        pt.temperature = 0.0f;
    } else {
        pt.pressure = _sumPressure / static_cast<float>(_count);
        pt.temperature = _sumTemperature / static_cast<float>(_count);
    }
    return pt;
}

float Pressure::averagePressure() const {
    return (_count > 0) ? (_sumPressure / static_cast<float>(_count)) : 0.0f;
}

float Pressure::averageTemperature() const {
    return (_count > 0) ? (_sumTemperature / static_cast<float>(_count)) : 0.0f;
}

void Pressure::add(PressureTemperature pt) {
    if (!_buf || _window == 0) return;

    if (_count < _window) {
        _sumPressure += pt.pressure;
        _sumTemperature += pt.temperature;
        _buf[_idx++] = pt;
        _count++;
        if (_idx == _window) _idx = 0;
    } else {
        // window full: subtract outgoing, add incoming
        _sumPressure -= _buf[_idx].pressure;
        _sumPressure += pt.pressure;
        _sumTemperature -= _buf[_idx].temperature;
        _sumTemperature += pt.temperature;
        _buf[_idx++] = pt;
        if (_idx == _window) _idx = 0;
    }
}

