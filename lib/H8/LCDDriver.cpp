#include <malloc.h>
#include <string.h>

#include "LCDDriver.h"

#ifdef CYBIKO_XTREME
#define LCD_INDEX *reinterpret_cast<volatile uint8_t *>(0x100000)
#define LCD_DATA  *reinterpret_cast<volatile uint8_t *>(0x100001)
#else
#define LCD_INDEX *reinterpret_cast<volatile uint8_t *>(0x600000)
#define LCD_DATA  *reinterpret_cast<volatile uint8_t *>(0x600001)
#endif

// registers
enum class LCDReg : uint8_t
{
    Control1 = 0,
    Control2,
    X,
    Y,
    Data,
    //5-11...
    Pal0 = 12,
    Pal1,
    Pal2,
    Pal3,
    Contrast
};

// control register values
enum Control1
{
    C1_ADC  = 1 << 0,
    C1_HOLT = 1 << 1,
    C1_REV  = 1 << 2,
    C1_AMP  = 1 << 3,
    C1_PWR  = 1 << 4,
    C1_STBY = 1 << 5,
    C1_DISP = 1 << 6,
    C1_RMW  = 1 << 7
};

enum Control2
{
    C2_BLK  = 1 << 0,
    C2_INC  = 1 << 1,
    C2_DTY0 = 1 << 2,
    C2_DTY1 = 1 << 3,
    C2_GRAY = 1 << 4,
    C2_WLS  = 1 << 5,
    C2_BIS0 = 1 << 6,
    C2_BIS1 = 1 << 7
};

static inline void writeReg(LCDReg reg, uint8_t val)
{
    LCD_INDEX = static_cast<uint8_t>(reg);
    LCD_DATA = val;
}

LCDDriver::LCDDriver() : minDirtyY(0), maxDirtyY(height)
{
    // power up
    writeReg(LCDReg::Control1, C1_PWR | C1_AMP | C1_ADC);

    // increment x
    writeReg(LCDReg::Control2, C2_INC);

    //writeReg(LCDReg::Contrast, 0xB);

    // enable display
    writeReg(LCDReg::Control1, C1_DISP | C1_PWR | C1_AMP | C1_ADC);

    // clear screen
    memset(framebuffer, 0, stride * height);
    updateDisplay();
}

void LCDDriver::setDirty(uint8_t minY, uint8_t maxY)
{
    if(minY < minDirtyY)
        minDirtyY = minY;

    if(maxY >= maxDirtyY)
        maxDirtyY = maxY >= height ? height : maxY + 1;
}

void LCDDriver::updateDisplay()
{
    if(maxDirtyY == 0)
        return;

    auto p = framebuffer;

    // swap y
    for(int8_t y = height - 1; y >= 0; y--)
    {
        writeReg(LCDReg::Y, y);

        LCD_INDEX = static_cast<uint8_t>(LCDReg::Data);

        for(int x = 0; x < stride; x++)
            LCD_DATA = *(p++);
    }

    maxDirtyY = 0;
}