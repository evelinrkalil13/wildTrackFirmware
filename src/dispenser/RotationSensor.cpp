#include "RotationSensor.h"

volatile uint32_t RotationSensor::_count = 0;

void RotationSensor::begin(int pin) {
    _pin = pin;
    pinMode(pin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(pin), _isr, CHANGE);
    Serial.print("[RotationSensor] GPIO"); Serial.println(pin);
}

void IRAM_ATTR RotationSensor::_isr() {
    _count++;
}

uint32_t RotationSensor::getPulseCount() const {
    noInterrupts();
    uint32_t c = _count;
    interrupts();
    return c;
}

void RotationSensor::resetCount() {
    noInterrupts();
    _count = 0;
    interrupts();
}
