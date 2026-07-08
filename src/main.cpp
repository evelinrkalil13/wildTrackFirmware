#include <Arduino.h>
#include "config/FirmwareConfig.h"
#include "config/DeviceConfig.h"
#include "config/Pins.h"
#include "diagnostics/DeviceInfoService.h"
#include "storage/ConfigStorage.h"
#include "provisioning/ProvisioningService.h"
#include "wifi/WifiService.h"
#include "mqtt/MqttService.h"
#include "sensors/DistanceSensor.h"
#include "sensors/EnvironmentSensor.h"
#include "scale/ScaleSensor.h"
#include "core/PresenceService.h"
#include "rfid/RfidReader.h"
#include "dispenser/DispenserMotor.h"
#include "dispenser/RotationSensor.h"
#include "dispenser/LimitSwitch.h"
#include "dispenser/DispenserService.h"
#include "core/TimeService.h"
#include "telemetry/TelemetryBuilder.h"
#include "camera/CameraService.h"
#include "core/FeedingSessionService.h"
#include "media/MediaUploadService.h"

static DeviceInfoService   deviceInfo;
static ConfigStorage       configStorage;
static ProvisioningService provisioner;
static WifiService         wifiService;
static MqttService         mqttService;
static DistanceSensor      distanceSensor;
static EnvironmentSensor   envSensor;
static ScaleSensor         scaleSensor;
static PresenceService     presenceService;
static RfidReader          rfidReader;
static DispenserMotor      dispenserMotor;
static RotationSensor      rotationSensor;
static LimitSwitch         limitSwitch;
static DispenserService    dispenserService;
static CameraService           cameraService;
static FeedingSessionService   sessionService;
static MediaUploadService      mediaService;
static constexpr uint16_t      API_PORT = 8000;
static DeviceConfig        config;
static uint32_t            lastAlive     = 0;
static TimeService         timeService;
static uint32_t            lastTelemetry = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);

    deviceInfo.begin();

    Serial.println("================================");
    Serial.print("Firmware:  "); Serial.println(deviceInfo.getFirmwareVersion());
    Serial.print("Serial:    "); Serial.println(deviceInfo.getSerialNumber());
    Serial.print("MAC:       "); Serial.println(deviceInfo.getMac());
    Serial.println("================================");

    configStorage.load(config);
    strncpy(config.serial_number, deviceInfo.getSerialNumber(), sizeof(config.serial_number) - 1);

    scaleSensor.begin(PIN_HX711_DOUT, PIN_HX711_SCK);
    if (config.hx711_calibration_factor != 0.0f)
        scaleSensor.setCalibrationFactor(config.hx711_calibration_factor);
    if (config.hx711_tare_offset != 0)
        scaleSensor.setTareOffset(config.hx711_tare_offset);

    provisioner.begin(config, configStorage);
    provisioner.attachScale(scaleSensor);

    distanceSensor.begin(PIN_DISTANCE_SENSOR);
    presenceService.begin(distanceSensor);
    envSensor.begin(PIN_DHT22);
    rfidReader.begin(PIN_RFID_RX);
    dispenserMotor.begin(PIN_MOTOR_IN1, PIN_MOTOR_IN2);
    rotationSensor.begin(PIN_ROTATION_SENSOR);
    limitSwitch.begin(PIN_LIMIT_SWITCH);
    dispenserService.begin(dispenserMotor, rotationSensor, limitSwitch);
    provisioner.attachDispenser(dispenserService);

    cameraService.begin();
    provisioner.attachCamera(cameraService);
    sessionService.begin(presenceService, cameraService, scaleSensor, rfidReader,
                         envSensor, timeService, deviceInfo, config.device_id);

    if (provisioner.isProvisioned()) {
        Serial.println("[Estado] PROVISIONADO");
        wifiService.begin(config.wifi_ssid, config.wifi_password);
        mqttService.begin(config.mqtt_host, config.mqtt_port,
                          config.device_id, config.serial_number);
        timeService.begin();
    } else {
        Serial.println("[Estado] PROVISIONING_REQUIRED");
    }
}

void loop() {
    provisioner.tick();
    presenceService.tick();
    rfidReader.tick();
    dispenserService.tick();
    sessionService.tick();
    envSensor.read();

    if (!provisioner.isProvisioned()) return;

    wifiService.tick();
    if (wifiService.isConnected()) mqttService.tick();

    uint32_t now = millis();
    if (now - lastAlive >= ALIVE_INTERVAL_MS) {
        lastAlive = now;
        float peso = scaleSensor.readWeightGrams();
        Serial.print("[");
        Serial.print(deviceInfo.getUptimeMs() / 1000);
        Serial.print("s]");
        Serial.print(" dist="); Serial.print(presenceService.lastDistanceCm(), 1); Serial.print("cm");
        Serial.print(" pres="); Serial.print(presenceService.isPresent() ? "SI" : "NO");
        Serial.print(" | peso="); Serial.print(peso, 1); Serial.print("g");
        if (envSensor.isValid()) {
            Serial.print(" temp="); Serial.print(envSensor.getTemperatureC(), 1); Serial.print("C");
            Serial.print(" hum=");  Serial.print(envSensor.getHumidity(), 1);    Serial.print("%");
        }
        Serial.print(" | MQTT="); Serial.print(mqttService.isConnected() ? "OK" : "off");
        Serial.print(" | rot="); Serial.print(rotationSensor.getPulseCount());
        Serial.print(" enc="); Serial.print(digitalRead(PIN_ROTATION_SENSOR));
        Serial.println();
    }

    if (wifiService.isConnected() && mqttService.isConnected()) {
        if (sessionService.hasEvent()) {
            camera_fb_t* photo = sessionService.getPhoto();
            if (photo) {
                char photoUrl[200] = {};
                bool uploaded = mediaService.upload(
                    config.mqtt_host, API_PORT,
                    config.device_id, sessionService.getEventId(),
                    photo, photoUrl, sizeof(photoUrl));
                if (uploaded) sessionService.setPhotoUrl(photoUrl);
            }
            bool ok = mqttService.publish(mqttService.topics().events, sessionService.getEventJson());
            Serial.print("[SESSION] publish "); Serial.println(ok ? "[OK]" : "[FAIL — reintento]");
            if (ok) sessionService.clearEvent();
        }

        if (now - lastTelemetry >= 60000UL) {
            lastTelemetry = now;
            if (!timeService.isReady()) {
                Serial.println("[TELEMETRY] skipped: time not ready");
            } else {
                char payload[320];
                TelemetryBuilder::build(
                    payload, sizeof(payload),
                    config.device_id,
                    deviceInfo.getFirmwareVersion(),
                    deviceInfo.getRssi(),
                    envSensor.getTemperatureC(),
                    envSensor.getHumidity(),
                    envSensor.isValid(),
                    timeService.getIso8601()
                );
                const char* topic = mqttService.topics().telemetry;
                bool ok = mqttService.publish(topic, payload);
                Serial.print("[TELEMETRY] topic=");   Serial.println(topic);
                Serial.print("[TELEMETRY] payload="); Serial.println(payload);
                Serial.print("[TELEMETRY] ");         Serial.println(ok ? "[OK]" : "[FAIL]");
            }
        }
    }
}
