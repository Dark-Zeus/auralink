#include "User_Setup.h"
#include "thb.h"
#include "danger.h"
#include "lv_functions.h"

THB thbSensor(20);  // moving average over 20 samples

void updateTHBUI(bool force) {
    static uint32_t lastUpdate = 0;
    const uint32_t now = millis();

    // debounce
    if (!force && (now - lastUpdate) < UI_SENSOR_UPDATE_INTERVAL_MS) return;
    lastUpdate = now;

    const float temp_imm = thbSensor.readImmediateTemperature();
    const float hum_imm = thbSensor.readImmediateHumidity();
    const float pres_imm = thbSensor.readImmediatePressure();
    const float temp_avg = thbSensor.average().temperature;
    const float hum_avg = thbSensor.average().humidity;
    const float pres_avg = thbSensor.average().pressure;

    const ColorOpacity co_temp = getDangerColorTemperature(temp_avg);
    const ColorOpacity co_hum = getDangerColorHumidity(hum_avg);
    const ColorOpacity co_pres = getDangerColorPressure(pres_avg);

    LV_SAFE(ui_TemperatureContainer, {
        lv_obj_set_style_bg_color(ui_TemperatureContainer, co_temp.color, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(ui_TemperatureContainer, co_temp.opacity, LV_PART_MAIN | LV_STATE_DEFAULT);
    });

    LV_SAFE(ui_Temperature, {
        lv_label_set_text_fmt(ui_Temperature, "%.2f", temp_avg);
    });

    LV_SAFE(ui_RelativeHumidityContainer, {
        lv_obj_set_style_bg_color(ui_RelativeHumidityContainer, co_hum.color, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(ui_RelativeHumidityContainer, co_hum.opacity, LV_PART_MAIN | LV_STATE_DEFAULT);
    });

    LV_SAFE(ui_RelativeHumidity, {
        lv_label_set_text_fmt(ui_RelativeHumidity, "%.2f", hum_avg);
    });

    LV_SAFE(ui_PressureContainer, {
        lv_obj_set_style_bg_color(ui_PressureContainer, co_pres.color, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(ui_PressureContainer, co_pres.opacity, LV_PART_MAIN | LV_STATE_DEFAULT);
    });

    LV_SAFE(ui_Pressure, {
        lv_label_set_text_fmt(ui_Pressure, "%.2f", pres_avg);
    });

    Serial.printf("[THB]: temp_imm=%.2f hum_imm=%.2f pres_imm=%.2f | temp_avg=%.2f hum_avg=%.2f pres_avg=%.2f\n",
                  temp_imm, hum_imm, pres_imm,
                  temp_avg, hum_avg, pres_avg);
}

THB::THB(size_t window) : _bme(), _window(window) {
    _buf = (_window > 0) ? new THBData[_window]() : nullptr;
    reset();
}

THB::~THB() {
    delete[] _buf;
    _buf = nullptr;
}

bool THB::begin(uint8_t addr, TwoWire* bus) {
    // Initialize BME280 sensor
    if (!_bme.begin(addr, bus)) {
        Serial.println("[BME280] Could not find a valid BME280 sensor, check wiring!");
        return false;
    }
    return true;
}

void THB::read() {
    THBData data;
    data.temperature = _bme.readTemperature();
    data.humidity = _bme.readHumidity();
    data.pressure = _bme.readPressure() / 100.0f;  // convert to hPa

    _add(data);
}

void THB::reset() {
    _idx = 0;
    _count_tmp = 0;
    _count_hum = 0;
    _count_pres = 0;
    _sumTemp = 0.0f;
    _sumHum = 0.0f;
    _sumPres = 0.0f;
    if (_buf && _window) {
        for (size_t i = 0; i < _window; ++i) {
            _buf[i].temperature = NAN;
            _buf[i].humidity = NAN;
            _buf[i].pressure = NAN;
        }
    }
}

void THB::_add(THBData v) {
    if (!_buf || _window == 0) return;

    // Temperature
    if (!isnan(v.temperature)) {
        if (_count_tmp < _window) {
            _sumTemp += v.temperature;
            _buf[_idx].temperature = v.temperature;
            _count_tmp++;
        } else {
            _sumTemp -= _buf[_idx].temperature;
            _sumTemp += v.temperature;
            _buf[_idx].temperature = v.temperature;
        }
    }

    // Humidity
    if (!isnan(v.humidity)) {
        if (_count_hum < _window) {
            _sumHum += v.humidity;
            _buf[_idx].humidity = v.humidity;
            _count_hum++;
        } else {
            _sumHum -= _buf[_idx].humidity;
            _sumHum += v.humidity;
            _buf[_idx].humidity = v.humidity;
        }
    }

    // Pressure
    if (!isnan(v.pressure)) {
        if (_count_pres < _window) {
            _sumPres += v.pressure;
            _buf[_idx].pressure = v.pressure;
            _count_pres++;
        } else {
            _sumPres -= _buf[_idx].pressure;
            _sumPres += v.pressure;
            _buf[_idx].pressure = v.pressure;
        }
    }

    // Advance index
    _idx++;
    if (_idx == _window) _idx = 0;
}

THB::THBData THB::readImmediate() {
    THBData data;
    data.temperature = _bme.readTemperature();
    data.humidity = _bme.readHumidity();
    data.pressure = _bme.readPressure() / 100.0f;  // convert to hPa
    return data;
}

float THB::readImmediateTemperature() {
    return _bme.readTemperature();
}

float THB::readImmediateHumidity() {
    return _bme.readHumidity();
}

float THB::readImmediatePressure() {
    return _bme.readPressure() / 100.0f;  // convert to hPa
}

THB::THBData THB::average() const {
    THBData data;
    data.temperature = (_count_tmp > 0) ? (_sumTemp / static_cast<float>(_count_tmp)) : NAN;
    data.humidity = (_count_hum > 0) ? (_sumHum / static_cast<float>(_count_hum)) : NAN;
    data.pressure = (_count_pres > 0) ? (_sumPres / static_cast<float>(_count_pres)) : NAN;
    return data;
}

float THB::averageTemperature() const {
    return (_count_tmp > 0) ? (_sumTemp / static_cast<float>(_count_tmp)) : NAN;
}

float THB::averageHumidity() const {
    return (_count_hum > 0) ? (_sumHum / static_cast<float>(_count_hum)) : NAN;
}

float THB::averagePressure() const {
    return (_count_pres > 0) ? (_sumPres / static_cast<float>(_count_pres)) : NAN;
}
