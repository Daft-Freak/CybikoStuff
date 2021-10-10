#ifndef PCF8593_H
#define PCF8593_H

#include <sys/time.h>

#include "H8CPU.h"

// Port F 1, 6 (extreme) 1, 0 (classic)
// 19 dec 2019
class PCF8593 final : public IODevice
{
public:
    PCF8593(int sdaBit, int sclBit);

    void updateTime();

protected:
    uint8_t read() override;
    void write(uint8_t val) override;
    void setDirection(uint8_t dir) override;

    void update();

    void recvByte();
    void sendByte();

    // i2c
    int sdaBit, sclBit;

    bool sdaState = true, sclState = false;
    bool sdaOut = false;

    bool lastSDA = false;
    bool start = false, gotAddr = false, ack = false, isWrite = false;
    int recvBit;
    uint8_t sendData, recvData;

    // pcf8593 specific
    bool gotReg = false;
    int regAddr = 0;

    uint8_t control = 0;

    struct timeval tv{0, 0};
    struct tm localTime;
};
#endif
