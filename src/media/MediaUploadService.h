#pragma once
#include <Arduino.h>
#include "esp_camera.h"

class MediaUploadService {
public:
    bool upload(const char* host, uint16_t port,
                const char* deviceId, const char* eventId,
                camera_fb_t* fb,
                char* outUrl, size_t urlLen);
};
