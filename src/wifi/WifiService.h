#pragma once
#include <Arduino.h>

class WifiService {
public:
    void   begin(const char* ssid, const char* password);
    void   tick();
    bool   isConnected() const;
    String getIp()       const;
    int    getRssi()     const;

private:
    const char* _ssid         = nullptr;
    const char* _password     = nullptr;
    bool        _wasConnected = false;
    uint32_t    _lastAttempt  = 0;

    static constexpr uint32_t RETRY_INTERVAL_MS = 30000;
};
