#pragma once
#include <stddef.h>

class TelemetryBuilder {
public:
    static void build(
        char* buf, size_t bufSize,
        const char* deviceId,
        const char* firmwareVersion,
        int rssiDbm,
        float tempC, float humPct, bool envValid,
        const char* timestamp
    );
};
