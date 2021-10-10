#ifndef LCDDEVICE_H
#define LCDDEVICE_H

#include "H8CPU.h"

class LCDDevice final : public ExternalAddressDevice
{
public:
    LCDDevice();

    uint8_t read(uint32_t addr) override;
    void write(uint32_t addr, uint8_t val) override;

    void convertDisplay(uint8_t *rgbaData);

protected:
    uint8_t regIndex;

    uint8_t control1, control2;
    uint8_t x, y;

    uint8_t mem[4000]{0};

    uint8_t pal[4];
};

#endif
