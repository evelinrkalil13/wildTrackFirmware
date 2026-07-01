#pragma once
#include "config/DeviceConfig.h"

class ConfigStorage {
public:
    // Returns true if device_id is present (device is provisioned).
    bool load(DeviceConfig& config);

    bool save(const DeviceConfig& config);
    bool clear();
};
