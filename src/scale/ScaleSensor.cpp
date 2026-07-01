#include "ScaleSensor.h"

void ScaleSensor::begin(int pinDout, int pinSck) {
    _hx711.begin((byte)pinDout, (byte)pinSck);
    _hx711.set_scale(1.0f);
    Serial.print("[ScaleSensor] HX711 DOUT="); Serial.print(pinDout);
    Serial.print(" SCK="); Serial.println(pinSck);
}

bool ScaleSensor::isReady() { return _hx711.is_ready(); }

long ScaleSensor::tare() {
    _hx711.tare(10);
    long offset = _hx711.get_offset();
    Serial.print("[ScaleSensor] Tare. Offset: "); Serial.println(offset);
    return offset;
}

void ScaleSensor::setTareOffset(long offset) {
    _hx711.set_offset(offset);
    Serial.print("[ScaleSensor] Offset cargado: "); Serial.println(offset);
}

void ScaleSensor::setCalibrationFactor(float factor) {
    _calibrationFactor = factor;
    _hx711.set_scale(factor);
    Serial.print("[ScaleSensor] Factor cargado: "); Serial.println(factor, 4);
}

float ScaleSensor::getCalibrationFactor() const { return _calibrationFactor; }

float ScaleSensor::calibrateWith(float knownGrams) {
    if (knownGrams <= 0.0f) return _calibrationFactor;
    _hx711.set_scale(1.0f);
    float raw = _hx711.get_units(10);
    float factor = raw / knownGrams;
    _hx711.set_scale(factor);
    _calibrationFactor = factor;
    Serial.print("[ScaleSensor] Calibrado con "); Serial.print(knownGrams, 1);
    Serial.print("g → factor: "); Serial.println(factor, 4);
    return factor;
}

float ScaleSensor::readWeightGrams() {
    if (!_hx711.is_ready()) return 0.0f;
    return _hx711.get_units(5);
}
