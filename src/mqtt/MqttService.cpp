#include "MqttService.h"
#include <Arduino.h>

MqttService::MqttService() : _client(_wifiClient) {}

void MqttService::begin(const char* host, uint16_t port,
                        const char* deviceId, const char* clientId,
                        const char* username, const char* password) {
    _host     = host;
    _port     = port;
    _deviceId = deviceId;
    _clientId = clientId;
    _username = (username && username[0]) ? username : nullptr;
    _password = (password && password[0]) ? password : nullptr;

    _topics.build(deviceId);
    _client.setServer(_host, _port);
    _client.setBufferSize(1024);

    Serial.print("[MQTT] Broker:     "); Serial.print(_host);
    Serial.print(":"); Serial.println(_port);
    Serial.print("[MQTT] Client ID:  "); Serial.println(_clientId);
    Serial.print("[MQTT] Status top: "); Serial.println(_topics.status);
}

bool MqttService::_connect() {
    const char* lwt = "{\"status\":\"offline\",\"reason\":\"unexpected_disconnect\"}";

    bool ok = _client.connect(_clientId,
                              _username, _password,
                              _topics.status, 0, false, lwt);
    if (ok) {
        _client.publish(_topics.status, "{\"status\":\"online\"}", false);
        Serial.println("[MQTT] Conectado. Publicado: online");
    } else {
        Serial.print("[MQTT] Fallo rc="); Serial.println(_client.state());
    }
    return ok;
}

void MqttService::tick() {
    if (!_client.connected()) {
        if (_wasConnected) {
            _wasConnected = false;
            _lastAttempt  = millis();
            Serial.println("[MQTT] Conexion perdida.");
        }

        uint32_t now = millis();
        if (_lastAttempt == 0 || now - _lastAttempt >= RETRY_INTERVAL_MS) {
            _lastAttempt = now;
            Serial.println("[MQTT] Conectando...");
            if (_connect()) _wasConnected = true;
        }
    } else {
        _wasConnected = true;
        _client.loop();
    }
}

bool MqttService::isConnected() { return _client.connected(); }

bool MqttService::publish(const char* topic, const char* payload, bool retained) {
    if (!_client.connected()) return false;
    return _client.publish(topic, payload, retained);
}
