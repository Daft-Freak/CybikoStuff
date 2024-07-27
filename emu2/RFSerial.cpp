#include <iostream>

#include "RFSerial.h"

#include "CRC.h"

RFSerial::RFSerial()
{
}

uint8_t RFSerial::read()
{
    auto val = writeQueue.front();
    writeQueue.pop_front();
    return val;
}

void RFSerial::write(uint8_t val)
{
    buf[bufOffset++] = val;

    if(!messageLen)
    {
        switch(val)
        {
            case 0x01: // some kind of command
                messageLen = 3;
                break;

            case 0x30: // short message
                messageLen = 52;
                break;
            
            case 0xCF: // long message
                messageLen = 202;
                break;

            default:
                printf("RFSerial: %02X\n", val);
                bufOffset = 0;
        }
    }
    else if(bufOffset == messageLen)
    {
        if(buf[0] == 0x01 && buf[1] == 2)
        {
            printf("RF channel = %i\n", buf[2]);
        }
        else if(buf[0] == 0x30 || buf[0] == 0xCF)
        {
            // these are mostly the same other than length
            bool longPkt = buf[0] == 0xCF;
            auto packetBuf = buf + 2;

            uint32_t dstAddr = (packetBuf[0] << 24) | (packetBuf[1] << 16) | (packetBuf[2] << 8) | (packetBuf[3]);
            uint32_t srcAddr = (packetBuf[4] << 24) | (packetBuf[5] << 16) | (packetBuf[6] << 8) | (packetBuf[7]);
            int channel = packetBuf[8] & 0x3F; // top bits = 0xC0?
            uint8_t type = packetBuf[9] >> 5; // ping, message, ack, sys
            uint8_t flags = packetBuf[9] & 0x1F; // ?  
            uint8_t index = packetBuf[10]; //?
            uint8_t unk11 = packetBuf[11];
            uint16_t dataCRC = (packetBuf[12] << 8) | packetBuf[13];
            uint16_t headCRC = (packetBuf[14] << 8) | packetBuf[15];
            int dataLen = longPkt ? 104 : 14;
            int footLen = longPkt ? 80 : 20;

            printf("RFSerial packet from %08X to %08X on chan %i\n", srcAddr, dstAddr, channel);
            printf("\ttype %i flags? %x index? %i unk11 %x data crc %04X header CRC %04X\n", type, flags, index, unk11, dataCRC, headCRC);
            printf("\tdata:");

            for(int i = 0; i < dataLen; i++)
            {
                bool newline = i > 0 && i % 16 == 0;
                printf("%s%02X", newline ? "\n\t      " : " ", packetBuf[i + 16]);
            }
            printf("\n");

            // this is calculated from the data somehow
            printf("\tfoot:");

            for(int i = 0; i < footLen; i++)
            {
                bool newline = i > 0 && i % 16 == 0;
                printf("%s%02X", newline ? "\n\t      " : " ", packetBuf[i + 16 + dataLen]);
            }
            printf("\n");

            // should be a delay here...
            writeQueue.push_back(0x03);
        }
        else
        {
            printf("RFSerial msg %02X\n", buf[0]);
            printf("\t");
            for(int i = 0; i < messageLen; i++)
                printf(" %02X", buf[i]);

            printf("\n");
        }

        messageLen = 0;
        bufOffset = 0;
    }
}

bool RFSerial::canRead()
{
    return !writeQueue.empty();;
}
