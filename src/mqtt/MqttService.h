#pragma once
#include <Arduino.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include "config/MqttTopics.h"

class MqttService {
public:
    MqttService();

    void begin(const char* host, uint16_t port,
               const char* deviceId, const char* clientId,
               const char* username = nullptr, const char* password = nullptr);
    void tick();
    bool isConnected();
    bool publish(const char* topic, const char* payload, bool retained = false);

    const MqttTopics& topics() const { return _topics; }

private:
    WiFiClient   _wifiClient;
    PubSubClient _client;
    MqttTopics   _topics;

    const char* _host     = nullptr;
    uint16_t    _port     = 1883;
    const char* _deviceId = nullptr;
    const char* _clientId = nullptr;
    const char* _username = nullptr;
    const char* _password = nullptr;
    bool        _wasConnected = false;
    uint32_t    _lastAttempt  = 0;

    static constexpr uint32_t RETRY_INTERVAL_MS = 10000;

    bool _connect();
};
