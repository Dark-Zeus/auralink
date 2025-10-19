#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <ui.h>

constexpr uint8_t BATTERY_CHARGER_PIN = 2;
constexpr uint8_t BATTERY_LEVEL_PIN = 7;

extern volatile bool chargerEvent;
extern volatile bool chargerLevel;

void IRAM_ATTR charger_isr();
void manageChargingState();
void updateBatteryUI(bool force = false);

class Battery {
   public:
    explicit Battery(size_t windowSize);
    ~Battery();

    float average() const;
    void reset();
    void read();
    size_t count() const { return _count; }
    size_t capacity() const { return _window; }
    float voltage(float v_min = 3.0f, float v_max = 4.2f,
                  uint16_t raw_min = 300, uint16_t raw_max = 2320) const;
    int percent(float v_min = 3.0f, float v_max = 4.2f, uint16_t raw_min = 300,
                uint16_t raw_max = 2320) const;
    void setCharging(bool isCharging) { _isCharging = isCharging; }
    bool isCharging() const { return _isCharging; }

   private:
    uint16_t* _buf = nullptr;
    size_t _window = 0;  // window size (N)
    size_t _idx = 0;     // next write index
    size_t _count = 0;   // how many valid samples in buffer
    uint32_t _sum = 0;   // running sum of current window
    bool _isCharging = false;

    void add(uint16_t v);
};

extern Battery battery;