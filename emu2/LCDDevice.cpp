// split out 20 dec 2019
// probably written in jan

#include <iostream>

#include "LCDDevice.h"

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

LCDDevice::LCDDevice() : control1(0), control2(0), x(0), y(0), pal{0, 0x0B, 0x17, 0x1F}
{

}

uint8_t LCDDevice::read(uint32_t addr)
{
    if(!(addr & 0x1))
    {
        std::cerr << "Attempt to read LCD index register!" << std::endl;
        return 0;
    }

    std::cout << "LCD r " << std::hex << static_cast<int>(regIndex) << std::dec << std::endl;

    return 0;
}

void LCDDevice::write(uint32_t addr, uint8_t val)
{
    //handle index write
    if(!(addr & 0x1))
    {
        regIndex = val;
        return;
    }

    switch(regIndex)
    {
        case 0:
            control1 = val;
            break;
        case 1:
            control2 = val;
        case 2:
            x = val;
            break;
        case 3:
            y = val;
            break;
        case 4:
            if(x + y * 40 > 4000)
                std::cerr << "OOB lcd write " << static_cast<int>(x) << ", " << static_cast<int>(y) << std::endl;
            else
                mem[x + y * 40] = val;

            if(control2 & C2_INC)
            {
                int max = (control1 & C2_WLS) ? 0x36 : 0x28;
                x = (x + 1) % max;
            }
            else
                y++;
            break;

        case 12:
        case 13:
        case 14:
        case 15:
            pal[regIndex - 12] = val;
            break;

        //case 16: contrast

        default:
            std::cout << "LCD w " << std::hex << static_cast<int>(regIndex) << " = " << static_cast<int>(val) << std::dec << std::endl;
    }
}

void LCDDevice::convertDisplay(uint8_t *rgbaData)
{
    //display off
    if((control1 & C1_STBY) || !(control1 & C1_DISP))
    {
        for(int i = 0; i < 160 * 100; i++)
        {
            rgbaData[i * 4 + 0] = 0;
            rgbaData[i * 4 + 1] = 0;
            rgbaData[i * 4 + 2] = 0;
            rgbaData[i * 4 + 3] = 0xFF;
        }
        return;
    }

    int inByte = 0;

    for(int y = 0; y < 100; y++)
    {
        for(int x = 0; x < 160 / 4; x++)
        {
            auto b = mem[inByte++];
            for(int x2 = 0; x2 < 4; x2++)
            {
                auto shift = 6 - x2 * 2;
                auto index = (b >> shift) & 0x3;

                uint8_t gray;

                if(control2 & C2_GRAY)
                    gray = pal[index] << 3;
                else
                    gray = index * 85;

                int outOff;
                if(control1 & C1_ADC)
                    outOff = ((x * 4 + x2) + (99 - y) * 160) * 4;
                else
                    outOff = ((x * 4 + x2) + y * 160) * 4;

                rgbaData[outOff++] = 0xFF - gray; //b
                rgbaData[outOff++] = 0xFF - gray; //g
                rgbaData[outOff++] = 0xFF - gray; //r
                rgbaData[outOff++] = 0xFF; //a
            }
        }
    }
}
