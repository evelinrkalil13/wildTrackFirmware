#pragma once
#include <Arduino.h>
#include "esp_camera.h"

class CameraService {
public:
    bool begin();
    camera_fb_t* capture();
    void         release(camera_fb_t* fb);
    bool         isReady() const;

private:
    bool _ready = false;
};
