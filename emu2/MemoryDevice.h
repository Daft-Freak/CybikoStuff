#ifndef MEMORYDEVICE_H
#define MEMORYDEVICE_H

#include "H8CPU.h"

class MemoryDevice final : public ExternalAddressDevice
{
public:
    MemoryDevice(uint32_t addressMask = 0x1FFFFF, bool debug = false);

    uint8_t read(uint32_t addr) override;
    void write(uint32_t addr, uint8_t val) override;

    uint8_t *getPtr(uint32_t &mask) override;

    bool loadFile(const char *filename, int offset = 0);
    void saveFile(const char *filename, int offset = 0, int length = -1);

    uint8_t *getData() {return mem;};

protected:
    bool debug;
    uint32_t addressMask;
    uint8_t mem[0x200000]{0};
};

#endif
