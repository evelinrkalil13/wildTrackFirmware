#pragma once
#include <Arduino.h>
#include "DispenserMotor.h"
#include "RotationSensor.h"
#include "LimitSwitch.h"

enum class DispenserState { IDLE, RUNNING_FORWARD, RUNNING_REVERSE, ERROR };

class DispenserService {
public:
    void begin(DispenserMotor& motor, RotationSensor& sensor, LimitSwitch& limitSwitch);
    void tick();

    bool dispense(uint8_t cycles = 3, uint16_t pulsesPerSide = 5);
    bool isDispensing()    const;
    bool hasError()        const;
    bool isFoodAvailable() const;
    void clearError();

private:
    DispenserMotor*  _motor  = nullptr;
    RotationSensor*  _sensor = nullptr;
    LimitSwitch*     _limit  = nullptr;

    uint8_t        _cycles        = 0;
    uint8_t        _currentCycle  = 0;
    uint16_t       _pulsesPerSide = 0;
    uint32_t       _dirStartTime  = 0;
    DispenserState _state         = DispenserState::IDLE;

    static constexpr uint32_t FORWARD_RUN_MS = 2500;
    static constexpr uint32_t DIR_TIMEOUT_MS = 35000;
    static constexpr uint32_t PAUSE_MS       = 150;

    void _startForward();
    void _startReverse();
    void _handleTimeout();
};
