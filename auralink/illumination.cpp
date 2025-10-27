#include "illumination.h"

#include <TFT_eSPI.h>
#include <lvgl.h>
#include <ui.h>

#include "User_Setup.h"
#include "danger.h"
#include "lv_functions.h"

BH1750 bh1750;

Illumination illuminationMeter(20);

void updateIlluminationUI(bool force) {
    static uint32_t lastUpdate = 0;
    static uint8_t lastFontIdx = 255;  // avoid redundant style writes
    uint32_t now = millis();

    if ((now - lastUpdate) < UI_SENSOR_UPDATE_INTERVAL_MS && !force) return;
    lastUpdate = now;

    float lux_imm = illuminationMeter.readImmediate();
    float lux_avg = illuminationMeter.average();

    // choose font by magnitude
    uint8_t fontIdx =
        (lux_avg < 1e3f) ? 14 : (lux_avg < 1e4f) ? 12
                            : (lux_avg < 1e5f)   ? 10
                                                 : 8;

    const lv_font_t* font =
        (fontIdx == 14) ? &lv_font_montserrat_14 : (fontIdx == 12) ? &lv_font_montserrat_12
                                               : (fontIdx == 10)   ? &lv_font_montserrat_10
                                                                   : &lv_font_montserrat_8;

    // set font only if changed
    if (fontIdx != lastFontIdx) {
        LV_SAFE_DO(ui_Illumination, { lv_obj_set_style_text_font(_o, font, LV_PART_MAIN | LV_STATE_DEFAULT); });
        lastFontIdx = fontIdx;
    }

    // set background color/opacity safely
    ColorOpacity co = getDangerColorIllumination(lux_avg);
    LV_SAFE_DO(ui_IlluminationContainer, {
        lv_obj_set_style_bg_color(_o, co.color, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(_o, co.opacity, LV_PART_MAIN | LV_STATE_DEFAULT);
    });

    // update label text safely
    LV_SAFE_DO(ui_Illumination, { lv_label_set_text_fmt(_o, "%.2f", lux_avg); });

    Serial.printf("[ILLUMINATION]: imm=%.2f avg=%.2f\n", lux_imm, lux_avg);
}

Illumination::Illumination(size_t windowSize) : _window(windowSize) {
    _buf = (_window > 0) ? new float[_window]
                         : nullptr;
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