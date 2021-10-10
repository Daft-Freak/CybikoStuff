#include <iostream>

#include "PCF8593.h"

PCF8593::PCF8593(int sdaBit, int sclBit) : sdaBit(sdaBit), sclBit(sclBit){}

void PCF8593::updateTime()
{
    gettimeofday(&tv, nullptr);

    localTime = *localtime(&tv.tv_sec);
}

uint8_t PCF8593::read()
{
    // sda has a pull-up, scl does not
    if(ack)
        return 0;
    else if(gotAddr && !isWrite)
    {
        bool bit = sendData & (0x80 >> (recvBit - 1));
        return bit ? (1 << sdaBit) : 0;
    }

    return 1 << sdaBit;
}

void PCF8593::write(uint8_t val)
{
    sdaState = val & (1 << sdaBit); //?
    sclState = val & (1 << sclBit);
    update();
}

void PCF8593::setDirection(uint8_t dir)
{
    sdaOut = dir & (1 << sdaBit);
    update();
}

void PCF8593::update()
{
    bool sda = !sdaOut;// || sdaState;

    if(lastSDA != sda && sclState)
    {
        // sda change while scl high

        if(!sda)
        {
            start = true;
            recvData = 0;
            recvBit = 0;
            gotAddr = ack = false;

            //
            gotReg = false;
        }
        else
        {
            start = false;
        }
    }

    // sch high, read sda
    if(lastSDA == sda && start && sclState)
    {
        // after 8th bit, ack is read, skip this bit
        if(recvBit == 8)
        {
            // first byte should be address
            if(!gotAddr)
            {
                if((recvData >> 1) == 0x51)
                {
                    gotAddr = true;
                    ack = true;
                    isWrite = !(recvData & 1);

                    if(!isWrite)
                        sendByte();
                }
                else
                    start = false; // ignore
            }
            else if(isWrite)
                recvByte();
            else if(!sda) // read ack
                sendByte();

            recvBit = 0;
        }
        else
        {
            ack = false;

            recvData <<= 1;
            if(sda)
                recvData |= 1;

            recvBit++;
        }

    }

    lastSDA = sda;
}

void PCF8593::recvByte()
{
    if(!gotReg)
    {
        regAddr = recvData;
        gotReg = ack = true;
    }
    else if(isWrite)
    {
        switch(regAddr)
        {
            case 0: //control/status
                control = recvData;
                break;
        }
        std::cout << "PCF8593 w " << regAddr << " = " << std::hex << static_cast<int>(recvData) << std::dec << "\n";
        regAddr++;
        ack = true;
    }
}

void PCF8593::sendByte()
{
    const auto bcd = [](int value)
    {
        return (value % 10) | ((value / 10) << 4);
    };

    switch(regAddr)
    {
        case 0: // control/status
            sendData = control;
            break;
        case 1: // 1/100 seconds
            sendData = bcd(tv.tv_usec / (1000 * 10));
            break;
        case 2: // seconds
            sendData = bcd(localTime.tm_sec);
            break;
        case 3: // minutes
            sendData = bcd(localTime.tm_min);
            break;
        case 4: // hours
            sendData = bcd(localTime.tm_hour);
            break;
        case 5: // year/date
            sendData = bcd(localTime.tm_mday)  | ((localTime.tm_year - 100) & 3) << 6;
            break;
        case 6: // weekday/month
        {
            int month = localTime.tm_mon + 1;
            sendData = bcd(month) | (localTime.tm_wday << 5);
            break;
        }

        case 7: // timer, but being used to store year
            sendData = bcd(localTime.tm_year - 100);
            break;

        default:
            std::cout << "PCF8593 r " << regAddr << "\n";
            sendData = 0;
    }
    regAddr++;
}
