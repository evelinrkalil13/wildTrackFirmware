#pragma once
#include <Arduino.h>

class DeviceInfoService {
public:
    void begin();

    const char* getFirmwareVersion() const;
    const char* getMac()             const;
    const char* getSerialNumber()    const;
    const char* getChipModel()       const;
    uint32_t    getChipRevision()    const;
    uint32_t    getFlashSizeMB()     const;
    uint32_t    getPsramSizeKB()     const;
    uint32_t    getUptimeMs()        const;
    int         getRssi()            const;  // populated in Step 4 (WiFi)

private:
    char     _mac[18];
    char     _serialNumber[12];
    char     _chipModel[16];
    uint32_t _chipRevision = 0;
    uint32_t _flashSizeMB  = 0;
    uint32_t _psramSizeKB  = 0;
};
