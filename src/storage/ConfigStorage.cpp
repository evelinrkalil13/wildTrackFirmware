#include "ConfigStorage.h"
#include <Preferences.h>
#include <cstring>

static const char* NVS_NS = "wildtrack";

bool ConfigStorage::load(DeviceConfig& config) {
    memset(&config, 0, sizeof(config));

    Preferences prefs;
    prefs.begin(NVS_NS, true); // read-only

    strncpy(config.device_id,               prefs.getString("device_id",  "").c_str(), sizeof(config.device_id)   - 1);
    strncpy(config.serial_number,           prefs.getString("serial_num", "").c_str(), sizeof(config.serial_number) - 1);
    strncpy(config.wifi_ssid,               prefs.getString("wifi_ssid",  "").c_str(), sizeof(config.wifi_ssid)    - 1);
    strncpy(config.wifi_password,           prefs.getString("wifi_pass",  "").c_str(), sizeof(config.wifi_password) - 1);
    strncpy(config.mqtt_host,               prefs.getString("mqtt_host",  "").c_str(), sizeof(config.mqtt_host)    - 1);
    strncpy(config.firmware_version,        prefs.getString("fw_version", "").c_str(), sizeof(config.firmware_version) - 1);
    config.mqtt_port                 = prefs.getUShort("mqtt_port",  0);
    config.hx711_calibration_factor  = prefs.getFloat("hx711_cal",  0.0f);
    config.hx711_tare_offset         = prefs.getInt("hx711_tare",   0);

    prefs.end();
    return config.device_id[0] != '\0';
}

bool ConfigStorage::save(const DeviceConfig& config) {
    Preferences prefs;
    if (!prefs.begin(NVS_NS, false)) return false;

    prefs.putString("device_id",  config.device_id);
    prefs.putString("serial_num", config.serial_number);
    prefs.putString("wifi_ssid",  config.wifi_ssid);
    prefs.putString("wifi_pass",  config.wifi_password);
    prefs.putString("mqtt_host",  config.mqtt_host);
    prefs.putString("fw_version", config.firmware_version);
    prefs.putUShort("mqtt_port",  config.mqtt_port);
    prefs.putFloat("hx711_cal",   config.hx711_calibration_factor);
    prefs.putInt("hx711_tare",    config.hx711_tare_offset);

    prefs.end();
    return true;
}

bool ConfigStorage::clear() {
    Preferences prefs;
    if (!prefs.begin(NVS_NS, false)) return false;
    bool ok = prefs.clear();
    prefs.end();
    return ok;
}
