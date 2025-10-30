#include "time_source.h"

#include <lvgl.h>
#include <ui.h>

#include "User_Setup.h"
#include "lv_functions.h"

TimeSource timeSource;

void updateTimeSourceUI(bool force) {
    static uint32_t lastUpdate = 0;
    static lv_obj_t* ui_sd_Time = nullptr;
    static lv_obj_t* ui_dq_Time = nullptr;
    static lv_obj_t* ui_es_Time = nullptr;
    uint32_t now = millis();

    if ((now - lastUpdate) < UI_SENSOR_UPDATE_INTERVAL_MS && !force) return;
    lastUpdate = now;

    if (!timeSource.isAvailable()) {
        Serial.println("[TIMESOURCE]: RTC not available; skipping UI update.");
        return;
    }

    LV_TRY_FIND(ui_sd_Time, ui_SDNotificationBar, UI_COMP_NOTIFICATIONBAR_TIMECONTAINER_TIME);
    LV_TRY_FIND(ui_dq_Time, ui_DQNotificationBar, UI_COMP_NOTIFICATIONBAR_TIMECONTAINER_TIME);
    LV_TRY_FIND(ui_es_Time, ui_ESNotificationBar, UI_COMP_NOTIFICATIONBAR_TIMECONTAINER_TIME);
    if (!lv_obj_ok(ui_sd_Time) && !lv_obj_ok(ui_dq_Time) && !lv_obj_ok(ui_es_Time)) {
        return;
    }

    time_t epoch = timeSource.getEpoch();
    struct tm* tm_info = localtime(&epoch);
    char buffer[6];
    strftime(buffer, sizeof(buffer), "%H:%M", tm_info);

    LV_SAFE(ui_sd_Time, { lv_label_set_text(ui_sd_Time, buffer); });
    LV_SAFE(ui_dq_Time, { lv_label_set_text(ui_dq_Time, buffer); });
    LV_SAFE(ui_es_Time, { lv_label_set_text(ui_es_Time, buffer); });    
}

TimeSource::TimeSource() : _available(false) {
    // nothing to do here
}

TimeSource::~TimeSource() {
    _rtc.~RTC_DS3231();
    _available = false;
}

bool TimeSource::begin(TwoWire* bus, long gmtOffsetSec, const String& ntpServer) {
    _gmtOffsetSec = gmtOffsetSec;
    _ntpServer = ntpServer;

    if (!_rtc.begin(bus)) {
        Serial.println("[TIMESOURCE] Could not find RTC");
        _available = false;
        return false;
    }

    if (_rtc.lostPower()) {
        Serial.println("[TIMESOURCE] RTC lost power, setting time to compile time");
        _rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    _available = true;
    Serial.println("[TIMESOURCE] RTC initialized successfully");
    return true;
}

bool TimeSource::isAvailable() const {
    return _available;
}

uint32_t TimeSource::getEpoch() {
    if (!_available) return 0;
    return _rtc.now().unixtime();
}

void TimeSource::syncCurrentTime() {
    if (!_available) return;
    if (!_ntpSynced) {
        _updateFromNTPTime();
        _ntpSynced = true;
    }
}

void TimeSource::setCurrentTime(time_t t) {
    if (!_available) return;
    _rtc.adjust(DateTime(t));
}

void TimeSource::_updateFromNTPTime() {
    // Use NTP to get current time, non-blocking
    configTime(_gmtOffsetSec, 0, _ntpServer.c_str());
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 5000)) {
        Serial.println("[TIMESOURCE] Failed to obtain NTP time.");
        return;
    }
    time_t t = mktime(&timeinfo);
    setCurrentTime(t);
    Serial.println("[TIMESOURCE] NTP time synchronized.");
}