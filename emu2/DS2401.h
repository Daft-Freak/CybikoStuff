#ifndef DS2401_H
#define DS2401_H

#include "H8CPU.h"

// port 6.2
// dec 2019
class DS2401 final : public IODevice
{
public:
    DS2401(H8CPU &cpu);

protected:
    uint8_t read() override;
    void write(uint8_t val) override;
    void setDirection(uint8_t dir) override;

    void update();

    void handleByte();

    H8CPU &cpu;

    const int bit = 2;
    bool prevState = true;
    bool pinVal = true, out = false;
    uint64_t lastStateTime = 0;

    int state = 0; // 1 = reset, 2 = reading data, 3 = writing data
    uint8_t recvData;
    int recvBit;

    uint64_t romData = 0x790123456789AB01ULL;
    int sendBit;
};

#endif
