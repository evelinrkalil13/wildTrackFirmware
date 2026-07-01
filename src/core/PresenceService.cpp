#include "PresenceService.h"

void PresenceService::begin(DistanceSensor& sensor) {
    _sensor = &sensor;
    Serial.print("[PresenceService] Umbral: ");
    Serial.print(THRESHOLD_CM); Serial.println(" cm");
}

void PresenceService::tick() {
    if (!_sensor) return;

    uint32_t now = millis();
    if (now - _lastSample < SAMPLE_INTERVAL_MS) return;
    _lastSample = now;

    _lastDistance = _sensor->readDistanceCm();
    bool close = (_lastDistance < THRESHOLD_CM);

    if (close) {
        _absenceCount = 0;
        if (_presenceCount < STABLE_COUNT) _presenceCount++;
    } else {
        _presenceCount = 0;
        if (_absenceCount < STABLE_COUNT) _absenceCount++;
    }

    bool wasPresent = _present;
    _present = (_presenceCount >= STABLE_COUNT);

    if (_present && !wasPresent) {
        Serial.print("[Presencia] DETECTADA — ");
        Serial.print(_lastDistance, 1); Serial.println(" cm");
    } else if (!_present && wasPresent) {
        Serial.println("[Presencia] RETIRADA");
    }
}

bool  PresenceService::isPresent()      const { return _present; }
float PresenceService::lastDistanceCm() const { return _lastDistance; }
