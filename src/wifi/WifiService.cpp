#include "WifiService.h"
#include <WiFi.h>

void WifiService::begin(const char* ssid, const char* password) {
    _ssid        = ssid;
    _password    = password;
    _lastAttempt = millis();

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(_ssid, _password);
    Serial.print("[WiFi] Conectando a "); Serial.println(_ssid);
}

void WifiService::tick() {
    bool connected = isConnected();

    if (connected && !_wasConnected) {
        _lastAttempt = millis();  // evita que el retry manual interfiera con auto-reconnect
        Serial.print("[WiFi] Conectado.  IP: "); Serial.print(getIp());
        Serial.print("  RSSI: "); Serial.print(getRssi()); Serial.println(" dBm");
    } else if (!connected && _wasConnected) {
        _lastAttempt = millis();  // reinicia el timer al perder conexion
        Serial.println("[WiFi] Conexion perdida.");
    }
    _wasConnected = connected;

    if (!connected) {
        uint32_t now = millis();
        if (now - _lastAttempt >= RETRY_INTERVAL_MS) {
            _lastAttempt = now;
            Serial.print("[WiFi] Reintentando conexion a "); Serial.println(_ssid);
            WiFi.begin(_ssid, _password);
        }
    }
}

bool   WifiService::isConnected() const { return WiFi.status() == WL_CONNECTED; }
String WifiService::getIp()       const { return WiFi.localIP().toString(); }
int    WifiService::getRssi()     const { return WiFi.RSSI(); }
