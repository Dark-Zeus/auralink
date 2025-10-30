#pragma once

#include <Arduino.h>
#include <Adafruit_BME280.h>

void updateTHBUI(bool force = false);

class THB {
   public:
    struct THBData {
        float temperature;
        float humidity;
        float pressure;
    };

    THB(size_t window = 20);
    ~THB();

    bool begin(uint8_t addr, TwoWire* bus);
    void reset();

    void read();
    void readTemperature();
    void readHumidity();
    void readPressure();

    THBData readImmediate();
    float readImmediateTemperature();
    float readImmediateHumidity();
    float readImmediatePressure();

    THBData average() const;
    float averageTemperature() const;
    float averageHumidity() const;
    float averagePressure() const;

   private:
    Adafruit_BME280 _bme;
    size_t _window = 0;
    size_t _idx = 0;
    size_t _count_tmp = 0;
    size_t _count_hum = 0;
    size_t _count_pres = 0;
    float _sumTemp = 0.0f;
    float _sumHum = 0.0f;
    float _sumPres = 0.0f;
    THBData* _buf = nullptr;

    void _add(THBData v);
};

extern THB thbSensor;  // moving average over 20 samples