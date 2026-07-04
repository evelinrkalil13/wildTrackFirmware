#pragma once
#include <HardwareSerial.h>

class RfidReader {
public:
    RfidReader();
    void        begin(int pinRx);
    void        tick();
    bool        hasTag()    const;
    const char* getTagId()  const;
    void        clearTag();

private:
    HardwareSerial _serial;
    char    _tagBuf[32];
    uint8_t _pos;
    bool    _reading;
    bool    _hasTag;
};
