#pragma once
#include <Adafruit_BMP085.h>
#include <Adafruit_Sensor.h>

void updatePressureUI(bool force = false);

struct PressureTemperature {
    float pressure;     // in hPa
    float temperature;  // in °C
};

class Pressure {
   public:
    Pressure(size_t window = 20);
    ~Pressure();
    bool begin();
    void read();
    void reset();
    void readPressure();
    void readTemperature();
    PressureTemperature readImmediate();
    float readImmediatePressure();
    float readImmediateTemperature();
    PressureTemperature average() const;
    float averagePressure() const;
    float averageTemperature() const;

   private:
    Adafruit_BMP085 _bmp;
    PressureTemperature* _buf = nullptr;
    size_t _window = 0;
    size_t _idx = 0;
    size_t _count = 0;
    float _sumPressure = 0.0f;
    float _sumTemperature = 0.0f;
    bool _ok = false;

    void add(PressureTemperature pt);
};

extern Pressure pressureSensor;  // moving average over 20 samples