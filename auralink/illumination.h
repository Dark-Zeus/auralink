#pragma once
#include <Arduino.h>
#include <BH1750.h>

extern BH1750 bh1750;

void updateIlluminationUI(bool force = false);

// BH1750 light sensor
class Illumination {
   public:
    explicit Illumination(size_t windowSize);
    ~Illumination();
    bool begin(uint8_t addr, TwoWire* bus = &Wire);
    void read();
    void reset();
    float readImmediate() const;

    float average() const;

   private:
    float* _buf = nullptr;
    size_t _window = 0;  // window size (N)
    size_t _idx = 0;     // next write index
    size_t _count = 0;   // how many valid samples in buffer
    double _sum = 0;   // running sum of current window
    uint8_t _pin;
    bool     _ok = false;

    void add(float v);
};

extern Illumination illuminationMeter;  // moving average over 10 samples