#include "illumination.h"
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <ui.h>

BH1750 bh1750;

Illumination illuminationMeter(20);

void updateIlluminationUI(bool force) {
    static uint32_t lastUpdate = 0;
    uint32_t now = millis();
    // 30s debounce
    if (now - lastUpdate < 300 && !force) {
        return;
    }
    lastUpdate = now;

    float lux_imm = illuminationMeter.readImmediate();
    float lux_avg = illuminationMeter.average();

    // update label text size based on digit count
    if (lux_avg < 1000.0f) {
        lv_obj_set_style_text_font(ui_Illumination, &lv_font_montserrat_14,
                                   LV_PART_MAIN | LV_STATE_DEFAULT);
    } else if (lux_avg < 10000.0f) {
        lv_obj_set_style_text_font(ui_Illumination, &lv_font_montserrat_12,
                                   LV_PART_MAIN | LV_STATE_DEFAULT);
    } else if (lux_avg < 100000.0f) {
        lv_obj_set_style_text_font(ui_Illumination, &lv_font_montserrat_10,
                                   LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        lv_obj_set_style_text_font(ui_Illumination, &lv_font_montserrat_8,
                                   LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    lv_label_set_text_fmt(ui_Illumination, "%.2f", lux_avg);
    Serial.printf("[ILLUMINATION]: imm=%.2f avg=%.2f\n", lux_imm, lux_avg);
}

Illumination::Illumination(size_t windowSize) : _window(windowSize) {
    _buf = (_window > 0) ? new float[_window]
                         : nullptr;  // CHANGED: float buffer, zero-init
    reset();
}

bool Illumination::begin(uint8_t addr, TwoWire* bus) {
    _ok = bh1750.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, addr, bus);
    if (_ok)
        delay(180);  // allow first conversion
    else
        Serial.println("[BH1750] Device is not configured!");
    return _ok;
}

Illumination::~Illumination() {
    delete[] _buf;
    _buf = nullptr;
}

void Illumination::read() {
    if (!_ok) return;
    // In continuous mode, data ready ~every 120–180ms; readLightLevel() is fine
    float lux = bh1750.readLightLevel();
    if (lux >= 0.0f && isfinite(lux)) add(lux);
}

float Illumination::readImmediate() const {
    if (!_ok) return -1.0f;  // guard in case someone calls too early
    float lux = bh1750.readLightLevel();
    return (lux >= 0.0f && isfinite(lux)) ? lux : -1.0f;
}

float Illumination::average() const {
    return (_count ? static_cast<float>(_sum) / static_cast<float>(_count)
                   : 0.0f);
}

void Illumination::add(float v) {
    if (!_buf || _window == 0) return;

    if (_count < _window) {
        _sum += v;
        _buf[_idx++] = v;
        _count++;
        if (_idx == _window) _idx = 0;
    } else {
        // window full: subtract outgoing, add incoming
        _sum -= _buf[_idx];
        _sum += v;
        _buf[_idx++] = v;
        if (_idx == _window) _idx = 0;
    }
}

void Illumination::reset() {
    _idx = 0;
    _count = 0;
    _sum = 0;
    if (_buf && _window) {
        for (size_t i = 0; i < _window; ++i) _buf[i] = 0;
    }
}