#pragma once
#include <stdint.h>

struct DeviceConfig {
    char     device_id[40];
    char     serial_number[12];
    char     wifi_ssid[33];
    char     wifi_password[65];
    char     mqtt_host[64];
    uint16_t mqtt_port;
    char     mqtt_username[32];
    char     mqtt_password[64];
    char     firmware_version[16];
    float    hx711_calibration_factor;
    int32_t  hx711_tare_offset;
};
