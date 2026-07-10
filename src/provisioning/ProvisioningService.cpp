#include "ProvisioningService.h"
#include "scale/ScaleSensor.h"
#include "dispenser/DispenserService.h"
#include "camera/CameraService.h"
#include <Arduino.h>
#include <strings.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>

void ProvisioningService::attachScale(ScaleSensor& scale) {
    _scale = &scale;
}

void ProvisioningService::attachDispenser(DispenserService& dispenser) {
    _dispenser = &dispenser;
}

void ProvisioningService::attachCamera(CameraService& camera) {
    _camera = &camera;
}

void ProvisioningService::begin(DeviceConfig& config, ConfigStorage& storage) {
    _config  = &config;
    _storage = &storage;
    memset(_buf, 0, sizeof(_buf));
    _pos = 0;

    if (!isProvisioned()) {
        Serial.println("[Provisioning] Dispositivo sin configuracion.");
        _printHelp();
    }
}

bool ProvisioningService::isProvisioned() const {
    return _config && _config->device_id[0] != '\0';
}

void ProvisioningService::tick() {
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (_pos > 0) {
                _buf[_pos] = '\0';
                _processLine(_buf);
                _pos = 0;
                memset(_buf, 0, sizeof(_buf));
            }
        } else if (_pos < sizeof(_buf) - 1) {
            _buf[_pos++] = c;
        }
    }
}

void ProvisioningService::_processLine(const char* line) {
    char cmd[16]  = {};
    char key[32]  = {};
    char val[128] = {};

    int parsed = sscanf(line, "%15s %31s %127[^\n]", cmd, key, val);

    if (strcasecmp(cmd, "set") == 0 && parsed >= 3) {
        if (strcmp(key, "device_id") == 0) {
            strncpy(_config->device_id, val, sizeof(_config->device_id) - 1);
            Serial.print("[OK] device_id = "); Serial.println(_config->device_id);

        } else if (strcmp(key, "wifi_ssid") == 0) {
            strncpy(_config->wifi_ssid, val, sizeof(_config->wifi_ssid) - 1);
            Serial.print("[OK] wifi_ssid = "); Serial.println(_config->wifi_ssid);

        } else if (strcmp(key, "wifi_pass") == 0) {
            strncpy(_config->wifi_password, val, sizeof(_config->wifi_password) - 1);
            Serial.println("[OK] wifi_pass = ****");

        } else if (strcmp(key, "mqtt_host") == 0) {
            strncpy(_config->mqtt_host, val, sizeof(_config->mqtt_host) - 1);
            Serial.print("[OK] mqtt_host = "); Serial.println(_config->mqtt_host);

        } else if (strcmp(key, "mqtt_port") == 0) {
            _config->mqtt_port = (uint16_t)atoi(val);
            Serial.print("[OK] mqtt_port = "); Serial.println(_config->mqtt_port);

        } else if (strcmp(key, "mqtt_user") == 0) {
            strncpy(_config->mqtt_username, val, sizeof(_config->mqtt_username) - 1);
            Serial.print("[OK] mqtt_user = "); Serial.println(_config->mqtt_username);

        } else if (strcmp(key, "mqtt_pass") == 0) {
            strncpy(_config->mqtt_password, val, sizeof(_config->mqtt_password) - 1);
            Serial.println("[OK] mqtt_pass = ****");

        } else {
            Serial.print("[ERR] Clave desconocida: "); Serial.println(key);
        }

    } else if (strcasecmp(cmd, "save") == 0) {
        if (_config->device_id[0]  == '\0') { Serial.println("[ERR] Falta device_id.");  return; }
        if (_config->wifi_ssid[0]  == '\0') { Serial.println("[ERR] Falta wifi_ssid.");  return; }
        if (_config->wifi_password[0] == '\0') { Serial.println("[ERR] Falta wifi_pass."); return; }
        if (_config->mqtt_host[0]  == '\0') { Serial.println("[ERR] Falta mqtt_host.");  return; }
        if (_config->mqtt_port     == 0)    { Serial.println("[ERR] Falta mqtt_port.");   return; }

        if (_storage->save(*_config)) {
            Serial.println("[OK] Config guardada. Reiniciando...");
            delay(500);
            ESP.restart();
        } else {
            Serial.println("[ERR] Error al guardar en flash.");
        }

    } else if (strcasecmp(cmd, "clear") == 0) {
        _storage->clear();
        Serial.println("[OK] Flash borrado. Reiniciando...");
        delay(500);
        ESP.restart();

    } else if (strcasecmp(cmd, "tare") == 0) {
        if (!_scale) { Serial.println("[ERR] Balanza no conectada."); return; }
        long offset = _scale->tare();
        _config->hx711_tare_offset = offset;
        _storage->save(*_config);
        Serial.println("[OK] Tare guardado en flash.");

    } else if (strcasecmp(cmd, "cal") == 0 && parsed >= 2) {
        if (!_scale) { Serial.println("[ERR] Balanza no conectada."); return; }
        float grams = atof(key);
        if (grams <= 0.0f) { Serial.println("[ERR] Gramos inválidos."); return; }
        float factor = _scale->calibrateWith(grams);
        _config->hx711_calibration_factor = factor;
        _storage->save(*_config);
        Serial.println("[OK] Factor guardado en flash.");

    } else if (strcasecmp(cmd, "dispense") == 0) {
        if (!_dispenser) { Serial.println("[ERR] Dispensador no conectado."); return; }
        uint8_t  ciclos = (parsed >= 2) ? (uint8_t)atoi(key)  : 3;
        uint16_t pulsos = (parsed >= 3) ? (uint16_t)atoi(val) : 5;
        if (ciclos == 0 || ciclos > 20)   { Serial.println("[ERR] Ciclos invalidos (1-20)."); return; }
        if (pulsos == 0 || pulsos > 100)  { Serial.println("[ERR] Pulsos invalidos (1-100)."); return; }
        if (!_dispenser->dispense(ciclos, pulsos)) {
            Serial.println("[ERR] No se pudo iniciar dispensacion.");
        }

    } else if (strcasecmp(cmd, "photo") == 0) {
        if (!_camera) { Serial.println("[ERR] Camara no conectada."); return; }
        camera_fb_t* fb = _camera->capture();
        if (!fb)      { Serial.println("[ERR] Captura fallida.");     return; }
        Serial.print("[OK] Foto: ");
        Serial.print(fb->width); Serial.print("x"); Serial.print(fb->height);
        Serial.print(" | "); Serial.print(fb->len / 1024.0f, 1); Serial.println(" KB");
        _camera->release(fb);

    } else if (strcasecmp(cmd, "status") == 0) {
        _printStatus();

    } else if (strcasecmp(cmd, "help") == 0) {
        _printHelp();

    } else {
        Serial.print("[ERR] Comando desconocido: "); Serial.println(cmd);
    }
}

void ProvisioningService::_printStatus() const {
    Serial.println("--- Config en memoria ---");
    Serial.print("  device_id:  "); Serial.println(_config->device_id[0]      ? _config->device_id  : "(vacio)");
    Serial.print("  wifi_ssid:  "); Serial.println(_config->wifi_ssid[0]      ? _config->wifi_ssid  : "(vacio)");
    Serial.print("  wifi_pass:  "); Serial.println(_config->wifi_password[0]  ? "****"              : "(vacio)");
    Serial.print("  mqtt_host:  "); Serial.println(_config->mqtt_host[0]      ? _config->mqtt_host     : "(vacio)");
    Serial.print("  mqtt_port:  "); Serial.println(_config->mqtt_port);
    Serial.print("  mqtt_user:  "); Serial.println(_config->mqtt_username[0] ? _config->mqtt_username : "(vacio)");
    Serial.print("  mqtt_pass:  "); Serial.println(_config->mqtt_password[0] ? "****"                 : "(vacio)");
    Serial.println("-------------------------");
}

void ProvisioningService::_printHelp() const {
    Serial.println("Comandos:");
    Serial.println("  set device_id <uuid>");
    Serial.println("  set wifi_ssid <nombre>");
    Serial.println("  set wifi_pass <password>");
    Serial.println("  set mqtt_host <host>");
    Serial.println("  set mqtt_port <puerto>");
    Serial.println("  set mqtt_user <usuario>");
    Serial.println("  set mqtt_pass <password>");
    Serial.println("  status  -> ver config actual");
    Serial.println("  save    -> guardar y reiniciar");
    Serial.println("  clear        -> borrar flash y reiniciar");
    Serial.println("  tare         -> tarar balanza y guardar offset");
    Serial.println("  cal <gramos> -> calibrar con peso conocido");
    Serial.println("  dispense [ciclos] [pulsos] -> dispensar (def: 3 ciclos, 5 pulsos/lado)");
    Serial.println("  photo        -> capturar foto y mostrar tamaño");
}
