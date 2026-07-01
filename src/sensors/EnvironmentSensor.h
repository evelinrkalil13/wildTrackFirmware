#pragma once
#include <Arduino.h>

class DHT;  // forward declaration — oculta dependencia de la librería

class EnvironmentSensor {
public:
    void  begin(int pin);
    bool  read();
    float getTemperatureC() const;
    float getHumidity()     const;
    bool  isValid()         const;

private:
    DHT*     _dht      = nullptr;
    float    _tempC    = NAN;
    float    _humidity = NAN;
    bool     _valid    = false;
    uint32_t _lastRead = 0;

    // DHT22 requiere mínimo 2 segundos entre lecturas
    static constexpr uint32_t MIN_INTERVAL_MS = 2500;
};
