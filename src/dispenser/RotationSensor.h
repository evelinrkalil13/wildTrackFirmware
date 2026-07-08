#pragma once
#include <Arduino.h>

class RotationSensor {
public:
    void     begin(int pin);
    uint32_t getPulseCount() const;
    void     resetCount();

private:
    static void     IRAM_ATTR _isr();
    static volatile uint32_t  _count;
    int _pin = -1;
};
