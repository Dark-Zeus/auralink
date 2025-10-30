#pragma once

#include <Arduino.h>

// UV sensor with 0-1V output connected to an analog pin

class UV {
   public:
    UV(size_t window);
    ~UV();

    bool begin( uint8_t pin);
    void reset();
    void read();
    float readImmediate();
    float average() const;

   private:
    uint8_t _pin = 0xFF;
    bool _ok = false;
    size_t _window = 0;
    size_t _idx = 0;    
    size_t _count = 0;
    float _sum = 0.0f;
    float* _buf = nullptr;
    void _add(float v);
    float _calculateUVIndex(uint16_t raw_adc) const;
};