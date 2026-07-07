#include "DispenserMotor.h"

void DispenserMotor::begin(int pinIn1, int pinIn2) {
    _pinIn1 = pinIn1;
    _pinIn2 = pinIn2;
    pinMode(_pinIn1, OUTPUT);
    pinMode(_pinIn2, OUTPUT);
    digitalWrite(_pinIn1, LOW);
    digitalWrite(_pinIn2, LOW);
    Serial.print("[DispenserMotor] IN1=GPIO"); Serial.print(pinIn1);
    Serial.print(" IN2=GPIO"); Serial.println(pinIn2);
}

void DispenserMotor::runForward() {
    if (_pinIn1 < 0) return;
    digitalWrite(_pinIn1, HIGH);
    digitalWrite(_pinIn2, LOW);
    _running = true;
    Serial.println("[DispenserMotor] Adelante");
}

void DispenserMotor::runReverse() {
    if (_pinIn1 < 0) return;
    digitalWrite(_pinIn1, LOW);
    digitalWrite(_pinIn2, HIGH);
    _running = true;
    Serial.println("[DispenserMotor] Atras");
}

void DispenserMotor::stop() {
    if (_pinIn1 < 0) return;
    digitalWrite(_pinIn1, LOW);
    digitalWrite(_pinIn2, LOW);
    _running = false;
    Serial.println("[DispenserMotor] Detenido");
}

bool DispenserMotor::isRunning() const { return _running; }
