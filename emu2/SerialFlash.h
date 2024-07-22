#ifndef SERIALFLASH_H
#define SERIALFLASH_H

#include "H8CPU.h"


// 18 july 2020
// used by classic
class SerialFlash final : public SerialDevice
{
public:
    uint8_t read() override;

    void write(uint8_t val) override;

    bool canRead() override;

    void setCS(bool val);

    bool loadFile(const char *filename, int offset = 0);
    void saveFile(const char *filename, int offset = 0, int length = -1);

private:
    bool cs = false;
    bool didWrite = false;
    bool wait = false;

    uint8_t opCode = 0;
    bool outputStatus = false;

    // buffer for commands
    uint8_t buf[8];
    int len = 0, off = 0;

    static const int pageSize = 264;

    uint32_t readAddr = 0xFFFFFFFF;
    int readOff = 0;

    uint32_t writeAddr = 0xFFFFFFFF;
    int writeOff = 0;
    uint8_t writeBuf[pageSize];

    uint8_t mem[0x84000];
};

#endif