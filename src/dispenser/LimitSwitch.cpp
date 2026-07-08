#include "LimitSwitch.h"

void LimitSwitch::begin(int pin) {
    _pin = pin;
    pinMode(pin, INPUT_PULLUP);
    Serial.print("[LimitSwitch] GPIO"); Serial.println(pin);
}

// NO + INPUT_PULLUP: LOW = presionado (alimento presente), HIGH = abierto (depósito vacío)
bool LimitSwitch::isClosed() const {
    return _pin >= 0 && digitalRead(_pin) == LOW;
}
