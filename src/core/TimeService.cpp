#include "TimeService.h"
#include <Arduino.h>
#include <time.h>
#include <cstring>

void TimeService::begin() {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
}

bool TimeService::isReady() const {
    struct tm t;
    if (!getLocalTime(&t, 0)) return false;
    return (t.tm_year + 1900) >= 2024;
}

const char* TimeService::getIso8601() const {
    struct tm t;
    if (!getLocalTime(&t, 0)) {
        strncpy(_buf, "1970-01-01T00:00:00Z", sizeof(_buf));
        return _buf;
    }
    strftime(_buf, sizeof(_buf), "%Y-%m-%dT%H:%M:%SZ", &t);
    return _buf;
}
