#include "battery.h"

#include <TFT_eSPI.h>
#include <lvgl.h>
#include <ui.h>

#include "User_Setup.h"
#include "lv_functions.h"

volatile bool chargerEvent = false;
volatile bool chargerLevel = false;

Battery battery(2000);

void manageChargingState() {
    // Read the state with interrupt protection
    noInterrupts();
    bool lvl = chargerLevel;
    chargerEvent = false;
    interrupts();

    battery.setCharging(lvl);
    bool charging = battery.isCharging();

    Serial.print("[BATTERY] Charging state changed: ");
    Serial.println(charging ? "CHARGING" : "NOT CHARGING");

    const uint32_t flag = LV_OBJ_FLAG_HIDDEN;

    // SD bar
    LV_SAFE_DO(LV_CHILD(ui_SDNotificationBar, UI_COMP_NOTIFICATIONBAR_BATTERYCONTAINER_CHARGINGICON), { charging ? lv_obj_clear_flag(_o, flag) : lv_obj_add_flag(_o, flag); });
    // DQ bar
    LV_SAFE_DO(LV_CHILD(ui_DQNotificationBar, UI_COMP_NOTIFICATIONBAR_BATTERYCONTAINER_CHARGINGICON), { charging ? lv_obj_clear_flag(_o, flag) : lv_obj_add_flag(_o, flag); });
    // ES bar
    LV_SAFE_DO(LV_CHILD(ui_ESNotificationBar, UI_COMP_NOTIFICATIONBAR_BATTERYCONTAINER_CHARGINGICON), { charging ? lv_obj_clear_flag(_o, flag) : lv_obj_add_flag(_o, flag); });

    Serial.println(charging ? "[BATTERY] Showing charging icon" : "[BATTERY] Hiding charging icon");
}

void updateBatteryUI(bool force) {
    static uint32_t lastUpdate = 0;
    uint32_t now = millis();
    // 300 ms debounce (comment said 30s; code is 300 ms)
    if ((now - lastUpdate) < 300 && !force) return;
    lastUpdate = now;

    float a = battery.average();  // averaged 0..4095
    int reading_ciel = 2400;
    int reading_floor = 1750;

    if (battery.isCharging()) {
        reading_ciel = 2400;
        reading_floor = 1750;
    }

    float v = battery.voltage(3.0f, 4.2f, reading_floor, reading_ciel);
    int p = battery.percent(3.0f, 4.2f, reading_floor, reading_ciel);

    lv_color_t c = (p <= 20)   ? lv_color_hex(0xC60047)
                   : (p <= 50) ? lv_color_hex(0xFFF500)
                               : lv_color_hex(0x2095F6);

    // --- Battery bars (3 places) ---
    // SD
    LV_SAFE_DO(LV_CHILD(ui_SDNotificationBar, UI_COMP_NOTIFICATIONBAR_BATTERYCONTAINER_BATTERY), {
        lv_obj_set_style_bg_color(_o, c, LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(_o, LV_OPA_COVER, LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_grad_dir(_o, LV_GRAD_DIR_NONE, LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_bar_set_value(_o, p, LV_ANIM_ON);
    });

    // DQ
    LV_SAFE_DO(LV_CHILD(ui_DQNotificationBar, UI_COMP_NOTIFICATIONBAR_BATTERYCONTAINER_BATTERY), {
        lv_obj_set_style_bg_color(_o, c, LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(_o, LV_OPA_COVER, LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_grad_dir(_o, LV_GRAD_DIR_NONE, LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_bar_set_value(_o, p, LV_ANIM_ON);
    });

    // ES
    LV_SAFE_DO(LV_CHILD(ui_ESNotificationBar, UI_COMP_NOTIFICATIONBAR_BATTERYCONTAINER_BATTERY), {
        lv_obj_set_style_bg_color(_o, c, LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(_o, LV_OPA_COVER, LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_grad_dir(_o, LV_GRAD_DIR_NONE, LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_bar_set_value(_o, p, LV_ANIM_ON);
    });

    LV_SAFE_DO(LV_CHILD(ui_SDNotificationBar, UI_COMP_NOTIFICATIONBAR_BATTERYCONTAINER_BATTERYTEXT), { lv_label_set_text_fmt(_o, "%d%%", p); });
    LV_SAFE_DO(LV_CHILD(ui_DQNotificationBar, UI_COMP_NOTIFICATIONBAR_BATTERYCONTAINER_BATTERYTEXT), {lv_label_set_text_fmt(_o, "%d%%", p);});
    LV_SAFE_DO(LV_CHILD(ui_ESNotificationBar, UI_COMP_NOTIFICATIONBAR_BATTERYCONTAINER_BATTERYTEXT), {lv_label_set_text_fmt(_o, "%d%%", p);});

    Serial.printf("[BATTERY]: raw=%.1f V=%.2fV %d%%\n", a, v, p);
}

void IRAM_ATTR charger_isr() {
    static uint32_t last = 0;
    uint32_t now = millis();
    if (now - last < 10) return;
    last = now;
    chargerLevel = digitalRead(BATTERY_CHARGER_PIN);
    chargerEvent = true;
}

Battery::Battery(size_t windowSize)
    : _window(windowSize) {
    _buf = (_window > 0) ? new uint16_t[_window] : nullptr;
    reset();
}

Battery::~Battery() {
    delete[] _buf;
    _buf = nullptr;
}

void Battery::reset() {
    _idx = 0;
    _count = 0;
    _sum = 0;
    if (_buf && _window) {
        for (size_t i = 0; i < _window; ++i) _buf[i] = 0;
    }
}

void Battery::read() {
    uint16_t v = analogRead(BATTERY_LEVEL_PIN);
    add(v);
}

float Battery::average() const {
    return (_count ? static_cast<float>(_sum) / static_cast<float>(_count) : 0.0f);
}

float Battery::voltage(float v_min, float v_max,
                       uint16_t raw_min, uint16_t raw_max) const {
    if (_count == 0 || raw_max <= raw_min) return v_min;
    float raw = average();
    // clamp to calibration span
    if (raw < raw_min) raw = raw_min;
    if (raw > raw_max) raw = raw_max;
    // linear map raw_min..raw_max -> v_min..v_max
    float t = (raw - raw_min) / float(raw_max - raw_min);
    return v_min + t * (v_max - v_min);
}

int Battery::percent(float v_min, float v_max,
                     uint16_t raw_min, uint16_t raw_max) const {
    // get VBAT from avg
    float v = voltage(v_min, v_max, raw_min, raw_max);

    // piecewise Li-ion curve (approximate, tweak as needed)
    // points: {voltage, percent}
    static const struct {
        float v;
        int p;
    } lut[] = {
        {3.00f, 0}, {3.20f, 5}, {3.50f, 20}, {3.70f, 40}, {3.85f, 60}, {4.00f, 80}, {4.10f, 90}, {4.20f, 100}};
    if (v <= lut[0].v) return 0;
    if (v >= lut[7].v) return 100;

    // find segment and interpolate
    for (int i = 0; i < 7; ++i) {
        if (v <= lut[i + 1].v) {
            float t = (v - lut[i].v) / (lut[i + 1].v - lut[i].v);
            int p = int(lut[i].p + t * (lut[i + 1].p - lut[i].p) + 0.5f);
            return p < 0 ? 0 : (p > 100 ? 100 : p);
        }
    }
    return 0;  // unreachable
}

void Battery::add(uint16_t v) {
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
