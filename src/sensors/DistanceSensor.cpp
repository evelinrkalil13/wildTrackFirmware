#include "DistanceSensor.h"
#include <cmath>

void DistanceSensor::begin(int pin) {
    _pin = pin;
    analogReadResolution(12);
    analogSetPinAttenuation(_pin, ADC_11db);
    Serial.print("[DistanceSensor] GPIO "); Serial.println(_pin);
}

int DistanceSensor::readRaw() {
    long sum = 0;
    for (int i = 0; i < 20; i++) {
        sum += analogRead(_pin);
        delay(2);
    }
    return (int)(sum / 20);
}

float DistanceSensor::readDistanceCm() {
    int   raw     = readRaw();
    float voltage = raw * (3.3f / 4095.0f);
    if (voltage < 0.3f) return 80.0f;

    // Fórmula empírica validada para GP2Y0A21YK0F
    float d = 29.988f * powf(voltage, -1.173f);
    if (d > 80.0f) d = 80.0f;
    return d;
}
