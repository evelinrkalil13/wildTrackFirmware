#include "EnvironmentSensor.h"
#include <DHT.h>
#include <cmath>

void EnvironmentSensor::begin(int pin) {
    _dht = new DHT(pin, DHT22);
    _dht->begin();
    Serial.print("[EnvironmentSensor] DHT22 en GPIO "); Serial.println(pin);
}

bool EnvironmentSensor::read() {
    if (!_dht) return false;

    uint32_t now = millis();
    if (now - _lastRead < MIN_INTERVAL_MS) return _valid;  // devuelve valor cacheado
    _lastRead = now;

    _humidity = _dht->readHumidity();
    _tempC    = _dht->readTemperature();
    _valid    = !isnan(_humidity) && !isnan(_tempC);

    if (!_valid) {
        Serial.println("[EnvironmentSensor] Error de lectura (NaN).");
    }
    return _valid;
}

float EnvironmentSensor::getTemperatureC() const { return _tempC; }
float EnvironmentSensor::getHumidity()     const { return _humidity; }
bool  EnvironmentSensor::isValid()         const { return _valid; }
