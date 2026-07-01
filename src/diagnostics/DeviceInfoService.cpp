#include "DeviceInfoService.h"
#include <esp_system.h>
#include <esp_chip_info.h>
#include <WiFi.h>
#include "config/FirmwareConfig.h"

void DeviceInfoService::begin() {
    // MAC from efuse (no WiFi needed)
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    snprintf(_mac, sizeof(_mac), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // Serial number derived from last 3 MAC bytes
    snprintf(_serialNumber, sizeof(_serialNumber), "WT-%02X%02X%02X",
             mac[3], mac[4], mac[5]);

    // Chip model and revision
    esp_chip_info_t chip;
    esp_chip_info(&chip);
    _chipRevision = chip.revision;

    const char* model;
    switch (chip.model) {
        case CHIP_ESP32:   model = "ESP32";    break;
        case CHIP_ESP32S2: model = "ESP32-S2"; break;
        case CHIP_ESP32S3: model = "ESP32-S3"; break;
        case CHIP_ESP32C3: model = "ESP32-C3"; break;
        default:           model = "Unknown";  break;
    }
    snprintf(_chipModel, sizeof(_chipModel), "%s", model);

    // Flash and PSRAM
    _flashSizeMB = ESP.getFlashChipSize() / (1024UL * 1024UL);
    _psramSizeKB  = ESP.getPsramSize()    / 1024UL;
}

const char* DeviceInfoService::getFirmwareVersion() const { return FIRMWARE_VERSION; }
const char* DeviceInfoService::getMac()             const { return _mac; }
const char* DeviceInfoService::getSerialNumber()    const { return _serialNumber; }
const char* DeviceInfoService::getChipModel()       const { return _chipModel; }
uint32_t    DeviceInfoService::getChipRevision()    const { return _chipRevision; }
uint32_t    DeviceInfoService::getFlashSizeMB()     const { return _flashSizeMB; }
uint32_t    DeviceInfoService::getPsramSizeKB()     const { return _psramSizeKB; }
uint32_t    DeviceInfoService::getUptimeMs()        const { return millis(); }
int         DeviceInfoService::getRssi()            const {
    if (WiFi.status() != WL_CONNECTED) return 0;
    return WiFi.RSSI();
}
