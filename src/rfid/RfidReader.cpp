#include "RfidReader.h"
#include <Arduino.h>
#include <cstring>

RfidReader::RfidReader() : _serial(1) {
    memset(_tagBuf, 0, sizeof(_tagBuf));
    _pos     = 0;
    _reading = false;
    _hasTag  = false;
}

void RfidReader::begin(int pinRx) {
    _serial.begin(9600, SERIAL_8N1, pinRx, -1);
    Serial.println("[RFID] Iniciado en GPIO " + String(pinRx) + " — esperando tags...");
}

void RfidReader::tick() {
    while (_serial.available()) {
        uint8_t b = _serial.read();

        if (b == 0x02) {
            memset(_tagBuf, 0, sizeof(_tagBuf));
            _pos     = 0;
            _reading = true;
            continue;
        }

        if (b == 0x03 && _reading) {
            _reading = false;
            _hasTag  = true;
            Serial.print("[RFID] TAG: ");
            Serial.println(_tagBuf);
            continue;
        }

        if (_reading && _pos < sizeof(_tagBuf) - 1) {
            _tagBuf[_pos++] = (char)b;
            _tagBuf[_pos]   = '\0';
        }
    }
}

bool        RfidReader::hasTag()   const { return _hasTag; }
const char* RfidReader::getTagId() const { return _tagBuf; }
void        RfidReader::clearTag()       { _hasTag = false; }
