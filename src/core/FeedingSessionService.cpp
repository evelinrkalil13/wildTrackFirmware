#include "FeedingSessionService.h"
#include "PresenceService.h"
#include "TimeService.h"
#include "../camera/CameraService.h"
#include "../scale/ScaleSensor.h"
#include "../rfid/RfidReader.h"
#include "../sensors/EnvironmentSensor.h"
#include "../diagnostics/DeviceInfoService.h"
#include <esp_random.h>

void FeedingSessionService::begin(PresenceService& presence, CameraService& camera,
                                   ScaleSensor& scale, RfidReader& rfid,
                                   EnvironmentSensor& env, TimeService& time,
                                   DeviceInfoService& deviceInfo, const char* deviceId) {
    _presence   = &presence;
    _camera     = &camera;
    _scale      = &scale;
    _rfid       = &rfid;
    _env        = &env;
    _time       = &time;
    _deviceInfo = &deviceInfo;
    _deviceId   = deviceId;
}

void FeedingSessionService::tick() {
    if (_hasEvent) return;

    if (!_active) {
        if (_rfid->hasTag()) _rfid->clearTag();  // discard tags outside sessions

        if (_presence->isPresent()) {
            if (_presenceStartMs == 0) _presenceStartMs = millis();
            else if (millis() - _presenceStartMs >= PRESENCE_STABLE_MS) _startSession();
        } else {
            _presenceStartMs = 0;
        }
        return;
    }

    // ACTIVE — capture RFID if arrives
    if (!_rfidDetected && _rfid->hasTag()) {
        strncpy(_rfidTag, _rfid->getTagId(), sizeof(_rfidTag) - 1);
        _rfid->clearTag();
        _rfidDetected = true;
        Serial.print("[Session] RFID: "); Serial.println(_rfidTag);
    }

    // Monitor absence
    if (!_presence->isPresent()) {
        if (_absenceStartMs == 0) _absenceStartMs = millis();
        else if (millis() - _absenceStartMs >= ABSENCE_CONFIRM_MS) _closeSession();
    } else {
        _absenceStartMs = 0;
    }
}

void FeedingSessionService::_startSession() {
    _generateUuidV4(_eventId, sizeof(_eventId));
    _weightInitial  = _scale->readWeightGrams();
    _rfidDetected   = false;
    _rfidTag[0]     = '\0';
    _absenceStartMs = 0;
    _envValid       = _env->isValid();
    _tempC          = _env->getTemperatureC();
    _humidityPct    = _env->getHumidity();

    _photoUrl[0] = '\0';
    if (_photo) { _camera->release(_photo); _photo = nullptr; }
    _photo      = _camera->capture();
    _photoCount = (_photo != nullptr) ? 1 : 0;

    _active = true;

    Serial.print("[Session] Inicio — event_id: "); Serial.println(_eventId);
    Serial.print("[Session] Peso inicial: "); Serial.print(_weightInitial, 1);
    Serial.print("g | Foto: "); Serial.println(_photoCount ? "OK" : "FAIL");
}

void FeedingSessionService::_closeSession() {
    if (_time->isReady()) {
        strncpy(_timestamp, _time->getIso8601(), sizeof(_timestamp) - 1);
    } else {
        strcpy(_timestamp, "1970-01-01T00:00:00Z");
    }
    _weightFinal = _scale->readWeightGrams();
    _rssi        = _deviceInfo->getRssi();
    _buildJson();

    _active          = false;
    _hasEvent        = true;
    _absenceStartMs  = 0;
    _presenceStartMs = 0;

    Serial.println("[Session] Completa");
    Serial.println("[Session] JSON:");
    Serial.println(_eventJson);
}

void FeedingSessionService::_buildJson() {
    char rfidTagJson[36];
    if (_rfidDetected && _rfidTag[0]) {
        snprintf(rfidTagJson, sizeof(rfidTagJson), "\"%s\"", _rfidTag);
    } else {
        strcpy(rfidTagJson, "null");
    }
    const char* readQuality = (_rfidDetected && _rfidTag[0]) ? "\"good\"" : "null";

    char tempJson[16], humJson[16];
    if (_envValid) {
        snprintf(tempJson, sizeof(tempJson), "%.1f", _tempC);
        snprintf(humJson,  sizeof(humJson),  "%.1f", _humidityPct);
    } else {
        strcpy(tempJson, "null");
        strcpy(humJson,  "null");
    }

    float consumed = _weightInitial - _weightFinal;
    if (consumed < 0.0f) consumed = 0.0f;

    char urlsJson[220];
    if (_photoUrl[0]) {
        snprintf(urlsJson, sizeof(urlsJson), "[\"%s\"]", _photoUrl);
    } else {
        strcpy(urlsJson, "[]");
    }

    snprintf(_eventJson, sizeof(_eventJson),
        "{"
        "\"event_id\":\"%s\","
        "\"event_type\":\"feeding_session\","
        "\"device_id\":\"%s\","
        "\"timestamp\":\"%s\","
        "\"rfid\":{\"detected\":%s,\"tag\":%s,\"read_quality\":%s},"
        "\"sensors\":{\"temperature_c\":%s,\"humidity_pct\":%s,"
        "\"initial_weight_g\":%.1f,\"final_weight_g\":%.1f,\"consumed_g\":%.1f},"
        "\"media\":{\"captured\":%u,\"urls\":%s},"
        "\"device_status\":{\"wifi_rssi_dbm\":%d,"
        "\"firmware_version\":\"%s\",\"battery_pct\":null}"
        "}",
        _eventId,
        _deviceId,
        _timestamp,
        _rfidDetected ? "true" : "false",
        rfidTagJson,
        readQuality,
        tempJson,
        humJson,
        _weightInitial,
        _weightFinal,
        consumed,
        (unsigned)_photoCount,
        urlsJson,
        _rssi,
        _deviceInfo->getFirmwareVersion()
    );
}

void FeedingSessionService::_generateUuidV4(char* out, size_t len) {
    uint8_t b[16];
    esp_fill_random(b, sizeof(b));
    b[6] = (b[6] & 0x0F) | 0x40;
    b[8] = (b[8] & 0x3F) | 0x80;
    snprintf(out, len,
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        b[0],b[1],b[2],b[3], b[4],b[5], b[6],b[7],
        b[8],b[9], b[10],b[11],b[12],b[13],b[14],b[15]);
}

bool         FeedingSessionService::hasEvent()     const { return _hasEvent; }
const char*  FeedingSessionService::getEventJson() const { return _eventJson; }
const char*  FeedingSessionService::getEventId()   const { return _eventId; }
camera_fb_t* FeedingSessionService::getPhoto()           { return _photo; }

void FeedingSessionService::setPhotoUrl(const char* url) {
    strncpy(_photoUrl, url, sizeof(_photoUrl) - 1);
    _photoUrl[sizeof(_photoUrl) - 1] = '\0';
    _buildJson();
}

void FeedingSessionService::clearEvent() {
    _hasEvent = false;
    if (_photo) { _camera->release(_photo); _photo = nullptr; }
    _photoCount = 0;
}
