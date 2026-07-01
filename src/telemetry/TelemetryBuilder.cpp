#include "TelemetryBuilder.h"
#include <cstdio>
#include <cstring>
#include <math.h>

void TelemetryBuilder::build(
    char* buf, size_t bufSize,
    const char* deviceId,
    const char* firmwareVersion,
    int rssiDbm,
    float tempC, float humPct, bool envValid,
    const char* timestamp)
{
    char tempStr[10], humStr[10];

    if (envValid && !isnan(tempC) && !isnan(humPct)) {
        snprintf(tempStr, sizeof(tempStr), "%.1f", tempC);
        snprintf(humStr,  sizeof(humStr),  "%.1f", humPct);
    } else {
        strncpy(tempStr, "null", sizeof(tempStr));
        strncpy(humStr,  "null", sizeof(humStr));
    }

    snprintf(buf, bufSize,
        "{"
        "\"device_id\":\"%s\","
        "\"timestamp\":\"%s\","
        "\"sensors\":{"
            "\"temperature_c\":%s,"
            "\"humidity_pct\":%s"
        "},"
        "\"device_status\":{"
            "\"wifi_rssi_dbm\":%d,"
            "\"firmware_version\":\"%s\","
            "\"battery_pct\":null"
        "}"
        "}",
        deviceId,
        timestamp,
        tempStr,
        humStr,
        rssiDbm,
        firmwareVersion
    );
}
