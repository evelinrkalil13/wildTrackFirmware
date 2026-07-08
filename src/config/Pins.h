#pragma once

// Camera (ESP32S3-EYE) uses GPIO 4,5,6,7,8,9,10,11,12,13,15,16,17,18
// All external sensors use camera-free GPIOs
static constexpr int PIN_DISTANCE_SENSOR = 14;  // was 4
static constexpr int PIN_HX711_DOUT      = 3;
static constexpr int PIN_HX711_SCK       = 2;
static constexpr int PIN_DHT22           = 21;  // was 5
static constexpr int PIN_LIMIT_SWITCH    = 40;  // was 6
static constexpr int PIN_MOTOR_IN1       = 38;
static constexpr int PIN_MOTOR_IN2       = 39;
static constexpr int PIN_ROTATION_SENSOR = 41;  // was 10
static constexpr int PIN_RFID_RX         = 42;  // was 18
static constexpr int PIN_RGB_LED         = 48;
