#pragma once
#include <Arduino.h>
#include <DHT.h>

struct TemperatureHumidity
{
    float temperature;
    float humidity;
};

void updateThermohygrometerUI(bool force = false);

class Thermohygrometer {
   public:
    explicit Thermohygrometer(size_t window);
    ~Thermohygrometer();

    bool begin(uint8_t pin, uint8_t type);
    void read();
    void readTemperature();
    void readHumidity();
    void reset();
    TemperatureHumidity readImmediate() const;
    float readImmediateTemperature() const;
    float readImmediateHumidity() const;
    TemperatureHumidity average() const;
    float averageTemperature() const;
    float averageHumidity() const;

   private:
    DHT* _dht = nullptr;
    uint8_t _pin;
    uint8_t _type;
    TemperatureHumidity* _buf = nullptr;
    size_t _window = 0;  // window size (N)
    size_t _idx = 0;     // next write index
    size_t _count = 0;   // how many valid samples in buffer
    double _sumTemp = 0;   // running sum of current window
    double _sumHum = 0;   // running sum of current window

    void add(TemperatureHumidity v);
};

extern Thermohygrometer thermohygrometer;