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
    void dumpPacket(const char *suffix, uint8_t *buf, bool longPkt);

    void eccInit();
    void eccCalc(uint8_t *data, bool longPkt);

    uint8_t rsMod(unsigned v);

    uint8_t buf[202];
    int bufOffset = 0;
    int messageLen = 0;

    // output
    std::deque<uint8_t> writeQueue;

    // lookups for reed-solomon ecc
    uint8_t rsExpTable[256], rsLogTable[256];
    uint8_t rsPoly20[21], rsPoly80[81];

    // network
    int recvFd, sendFd;
    void *sendAddr; // avoiding network headers

    uint8_t lastHead[16]; // used to avoid immediately receiving our own packet
};

#endif