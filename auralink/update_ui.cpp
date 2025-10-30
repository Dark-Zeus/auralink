#include "update_ui.h"
#include "lv_functions.h"
#include "User_Setup.h"
#include "danger.h"

#include <Arduino.h>
#include <lvgl.h>
#include <ui.h>

void updateEmailSummary(const char* summary) {
    LV_SAFE(ui_EmailSummaryLabel, {
        lv_label_set_text(ui_EmailSummaryLabel, summary);
    });
}

void updateDailyQuote(const char* quote) {
    LV_SAFE(ui_QuoteLabel, {
        lv_label_set_text(ui_QuoteLabel, quote);
    });
}

void updateUVIndexUI(UV& instance, bool force) {
    static uint32_t lastUpdate = 0;
    uint32_t now = millis();
    if ((now - lastUpdate) < UI_SENSOR_UPDATE_INTERVAL_MS && !force) return;
    lastUpdate = now;

    float uv_imm = instance.readImmediate();
    float uv_avg = instance.average();
    
    ColorOpacity co = getDangerColorUVIndex(uv_avg);

    LV_SAFE(ui_UVContainer, {
        lv_obj_set_style_bg_color(ui_UVContainer, co.color, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(ui_UVContainer, co.opacity, LV_PART_MAIN | LV_STATE_DEFAULT);
    });

    LV_SAFE(ui_UVV, {
        lv_label_set_text_fmt(ui_UVV, "%.2f", uv_avg);
    });

    Serial.printf("[UVINDEX]: imm=%.2f avg=%.2f\n", uv_imm, uv_avg);
}