#include "update_ui.h"

#include <lvgl.h>
#include <ui.h>

void updateEmailSummary(const char* summary) {
    lv_label_set_text(ui_EmailSummaryLabel, summary);
}

void updateDailyQuote(const char* quote) {
    lv_label_set_text(ui_QuoteLabel, quote);
}