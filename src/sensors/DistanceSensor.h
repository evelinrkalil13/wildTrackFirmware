#pragma once
#include <Arduino.h>

class DistanceSensor {
public:
    void  begin(int pin);
    int   readRaw();         // ADC 0-4095 (útil para calibrar)
    float readDistanceCm();  // distancia estimada en cm

private:
    int _pin = -1;
};
