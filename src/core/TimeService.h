#pragma once

class TimeService {
public:
    void begin();
    bool isReady() const;
    const char* getIso8601() const;

private:
    mutable char _buf[25];
};
