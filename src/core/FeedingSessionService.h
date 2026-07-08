#pragma once
#include <Arduino.h>
#include "esp_camera.h"

class PresenceService;
class CameraService;
class ScaleSensor;
class RfidReader;
class EnvironmentSensor;
class TimeService;
class DeviceInfoService;

class FeedingSessionService {
public:
    void begin(PresenceService& presence, CameraService& camera,
               ScaleSensor& scale, RfidReader& rfid,
               EnvironmentSensor& env, TimeService& time,
               DeviceInfoService& deviceInfo, const char* deviceId);
    void tick();

    bool         hasEvent()     const;
    const char*  getEventJson() const;
    const char*  getEventId()   const;
    camera_fb_t* getPhoto();
    void         setPhotoUrl(const char* url);
    void         clearEvent();

private:
    static constexpr uint32_t PRESENCE_STABLE_MS = 1500;
    static constexpr uint32_t ABSENCE_CONFIRM_MS = 3000;

    PresenceService*   _presence   = nullptr;
    CameraService*     _camera     = nullptr;
    ScaleSensor*       _scale      = nullptr;
    RfidReader*        _rfid       = nullptr;
    EnvironmentSensor* _env        = nullptr;
    TimeService*       _time       = nullptr;
    DeviceInfoService* _deviceInfo = nullptr;
    const char*        _deviceId   = nullptr;

    bool     _active          = false;
    bool     _hasEvent        = false;
    uint32_t _presenceStartMs = 0;
    uint32_t _absenceStartMs  = 0;

    char         _eventId[37]    = {};
    char         _timestamp[25]  = {};
    float        _weightInitial  = 0.0f;
    float        _weightFinal    = 0.0f;
    bool         _rfidDetected   = false;
    char         _rfidTag[32]    = {};
    float        _tempC          = 0.0f;
    float        _humidityPct    = 0.0f;
    bool         _envValid       = false;
    int          _rssi           = 0;
    uint8_t      _photoCount     = 0;
    camera_fb_t* _photo          = nullptr;

    char _photoUrl[200]   = {};
    char _eventJson[800]  = {};

    void _startSession();
    void _closeSession();
    void _buildJson();
    void _generateUuidV4(char* out, size_t len);
};
