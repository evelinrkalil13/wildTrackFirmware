#pragma once
#include <Arduino.h>
#include <HX711.h>

class ScaleSensor {
public:
    void  begin(int pinDout, int pinSck);
    bool  isReady();
    long  tare();
    void  setTareOffset(long offset);
    void  setCalibrationFactor(float factor);
    float getCalibrationFactor() const;
    float calibrateWith(float knownGrams);
    float readWeightGrams();

private:
    HX711 _hx711;
    float _calibrationFactor = 1.0f;
};
