#pragma once
#include "config/DeviceConfig.h"
#include "storage/ConfigStorage.h"

class ScaleSensor;      // forward declaration
class DispenserService; // forward declaration

class ProvisioningService {
public:
    void begin(DeviceConfig& config, ConfigStorage& storage);
    void attachScale(ScaleSensor& scale);
    void attachDispenser(DispenserService& dispenser);
    void tick();
    bool isProvisioned() const;

private:
    DeviceConfig*     _config     = nullptr;
    ConfigStorage*    _storage    = nullptr;
    ScaleSensor*      _scale      = nullptr;
    DispenserService* _dispenser  = nullptr;
    char           _buf[128];
    uint8_t        _pos = 0;

    void _processLine(const char* line);
    void _printStatus() const;
    void _printHelp()   const;
};
