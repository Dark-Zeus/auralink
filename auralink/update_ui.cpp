#include "update_ui.h"
#include "lv_functions.h"

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