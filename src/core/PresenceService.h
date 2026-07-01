#pragma once
#include <Arduino.h>
#include "sensors/DistanceSensor.h"

class PresenceService {
public:
    void begin(DistanceSensor& sensor);
    void tick();

    bool  isPresent()      const;
    float lastDistanceCm() const;

private:
    DistanceSensor* _sensor        = nullptr;
    bool            _present       = false;
    uint8_t         _presenceCount = 0;
    uint8_t         _absenceCount  = 0;
    float           _lastDistance  = 80.0f;
    uint32_t        _lastSample    = 0;

    // Ajusta THRESHOLD_CM según la fórmula calibrada de tu sensor
    static constexpr float    THRESHOLD_CM       = 10.0f;
    static constexpr uint8_t  STABLE_COUNT       = 3;
    static constexpr uint32_t SAMPLE_INTERVAL_MS = 100;
};
