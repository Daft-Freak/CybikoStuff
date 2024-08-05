#include <iostream>

#include "DS2401.h"

DS2401::DS2401(H8CPU &cpu) : cpu(cpu){}

uint8_t DS2401::read(uint32_t time)
{
    if(state == 3 && sendBit != -1)
        return (romData & (1ULL << sendBit)) ? (1 << bit) : 0;

    return state == 1 ? 0 : (1 << bit);
}

void DS2401::write(uint8_t val, uint32_t time)
{
    pinVal = val & (1 << bit);
    update(time);
}

void DS2401::setDirection(uint8_t dir, uint32_t time)
{
    out = dir & (1 << bit);

    update(time);
}

void DS2401::update(uint32_t time)
{
    // external pull-up, setting to input causes a high
    bool val = !out || pinVal;

    if(lastStateTime == 0)
        lastStateTime = time;

    if(val != prevState)
    {
        // ~18.432
        auto len = (time - lastStateTime) / 18;

        if(val == 1)
        {
            // low for at least 480us
            if(len > 480)
            {
                // reset
                state = 1;
            }
            else if(state == 1)
            {
                // start reaading byte
                state = 2;
                recvData = 0;
                recvBit = 0;
            }

            if(state == 2)
            {
                recvData >>= 1;

                if(len < 15)
                    recvData |= 0x80; // 1
                else if(len > 60)
                {} // 0

                recvBit++;

                if(recvBit == 8)
                {
                    std::cout << "one wire byte " << std::hex << static_cast<int>(recvData) << std::dec << std::endl;
                    state = 1;
                    handleByte();
                }
            }
            else if(state == 3)
                sendBit++;
        }

        lastStateTime = time;
    }

    prevState = val;
}

void DS2401::handleByte()
{
    if(recvData == 0x33)
    {
        std::cout << "DS2401 read rom" << std::endl;
        state = 3;
        sendBit = -1;
    }
}

