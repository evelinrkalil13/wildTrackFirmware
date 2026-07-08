#include "DispenserService.h"

void DispenserService::begin(DispenserMotor& motor, RotationSensor& sensor, LimitSwitch& limitSwitch) {
    _motor  = &motor;
    _sensor = &sensor;
    _limit  = &limitSwitch;
    Serial.print("[DispenserService] Alimento: ");
    Serial.println(isFoodAvailable() ? "DISPONIBLE" : "SIN ALIMENTO");
}

bool DispenserService::dispense(uint8_t cycles, uint16_t pulsesPerSide) {
    if (_state == DispenserState::ERROR) clearError();
    if (_state != DispenserState::IDLE) {
        Serial.println("[DISPENSER] Ocupado.");
        return false;
    }
    if (!isFoodAvailable()) {
        Serial.println("[DISPENSER] Sin alimento — cancelado.");
        return false;
    }
    _cycles        = cycles;
    _currentCycle  = 0;
    _pulsesPerSide = pulsesPerSide;
    Serial.print("[DISPENSER] Iniciando: ");
    Serial.print(cycles);
    Serial.print(" ciclos x ");
    Serial.print(pulsesPerSide);
    Serial.println(" pulsos/lado");
    _startForward();
    return true;
}

void DispenserService::_startForward() {
    _sensor->resetCount();
    _dirStartTime = millis();
    _motor->runForward();
    _state = DispenserState::RUNNING_FORWARD;
}

void DispenserService::_startReverse() {
    _motor->stop();
    delay(PAUSE_MS);
    _sensor->resetCount();
    _dirStartTime = millis();
    _motor->runReverse();
    _state = DispenserState::RUNNING_REVERSE;
}

void DispenserService::_handleTimeout() {
    _motor->stop();
    _state = DispenserState::ERROR;
    Serial.println("[DISPENSER] TIMEOUT — DISPENSER_ERROR");
}

void DispenserService::tick() {
    if (_state == DispenserState::IDLE || _state == DispenserState::ERROR) return;

    uint32_t pulses  = _sensor->getPulseCount();
    uint32_t elapsed = millis() - _dirStartTime;

    if (_state == DispenserState::RUNNING_FORWARD) {
        if (elapsed >= FORWARD_RUN_MS) {
            _startReverse();
        }
        return;
    }

    if (_state == DispenserState::RUNNING_REVERSE) {
        if (pulses >= _pulsesPerSide) {
            _currentCycle++;
            Serial.print("[DISPENSER] Ciclo "); Serial.print(_currentCycle);
            Serial.print("/"); Serial.println(_cycles);
            if (_currentCycle >= _cycles) {
                _motor->stop();
                _state = DispenserState::IDLE;
                Serial.println("[DISPENSER] Completo");
                return;
            }
            _motor->stop();
            delay(PAUSE_MS);
            _startForward();
            return;
        }
        if (elapsed >= DIR_TIMEOUT_MS) _handleTimeout();
    }
}

bool DispenserService::isDispensing() const {
    return _state == DispenserState::RUNNING_FORWARD ||
           _state == DispenserState::RUNNING_REVERSE;
}

bool DispenserService::hasError()        const { return _state == DispenserState::ERROR;  }
bool DispenserService::isFoodAvailable() const { return _limit && _limit->isClosed();     }
void DispenserService::clearError()            { if (hasError()) _state = DispenserState::IDLE; }
