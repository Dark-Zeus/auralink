#include "uv.h"

UV::UV(size_t window)
    : _window(window) {
    _buf = (_window > 0) ? new float[_window]() : nullptr;
    reset();
}

UV::~UV() {
    delete[] _buf;
    _buf = nullptr;
}

void UV::reset() {
    _idx = 0;
    _count = 0;
    _sum = 0.0f;
    if (_buf && _window) {
        for (size_t i = 0; i < _window; ++i) {
            _buf[i] = NAN;
        }
    }
}

bool UV::begin(uint8_t pin) {
    _pin = pin;
    analogSetPinAttenuation(_pin, ADC_11db);
    _ok = true;
    return _ok;
}

void UV::read() {
    if (!_ok) return;

    (void)analogRead(_pin);
    delayMicroseconds(50);

    uint16_t raw_adc = analogRead(_pin);
    float uvi = _calculateUVIndex(raw_adc);

    _add(uvi);
}

float UV::readImmediate() {
    if (!_ok) return -1.0f;

    (void)analogRead(_pin);
    delayMicroseconds(50);

    uint16_t raw_adc = analogRead(_pin);
    float uvi = _calculateUVIndex(raw_adc);

    return uvi;
}

float UV::average() const {
    return (_count ? static_cast<float>(_sum) / static_cast<float>(_count)
                   : 0.0f);
}

void UV::_add(float v){
    if (!_buf || _window == 0) return;

    if (_count < _window) {
        _sum += v;
        _buf[_idx++] = v;
        _count++;
        if (_idx == _window) _idx = 0;
    } else {
        _sum -= _buf[_idx];
        _sum += v;
        _buf[_idx++] = v;
        if (_idx == _window) _idx = 0;
    }
}

float UV::_calculateUVIndex(uint16_t raw_adc) const {
    float vAdc = (raw_adc / 4095.0f) * 3.3f;
    float uvi = vAdc * 10.0f;
    return uvi;
}