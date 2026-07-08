#pragma once
#include <Arduino.h>

class DispenserMotor {
public:
    void begin(int pinIn1, int pinIn2);
    void runForward();
    void runReverse();
    void stop();
    bool isRunning() const;

private:
    int  _pinIn1   = -1;
    int  _pinIn2   = -1;
    bool _running  = false;
};
