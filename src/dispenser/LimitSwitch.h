#pragma once
#include <Arduino.h>

class LimitSwitch {
public:
    void begin(int pin);
    bool isClosed() const;  // true = switch presionado (alimento disponible)

private:
    int _pin = -1;
};
