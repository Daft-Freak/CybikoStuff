#ifndef RFSERIAL_H
#define RFSERIAL_H

#include <deque>

#include "H8CPU.h"

// Serial0 on classic, 2 on xtreme
class RFSerial final : public SerialDevice
{
public:
    RFSerial();
    ~RFSerial();

    uint8_t read() override;

    void write(uint8_t val) override;

    bool canRead() override;

    void networkUpdate();

private:
    uint8_t buf[202];
    int bufOffset = 0;
    int messageLen = 0;

    // output
    std::deque<uint8_t> writeQueue;

    // network
    int recvFd, sendFd;
    void *sendAddr; // avoiding network headers
};

#endif