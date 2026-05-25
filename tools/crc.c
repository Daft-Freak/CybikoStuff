#include "crc.h"

uint32_t crc16(uint8_t *data, int length)
{
    uint16_t crc = 0xFFFF;

    for(int i = 0; i < length; i++)
    {
        crc ^= data[i];
        for(int j = 0; j < 8; j++)
        {
            bool bit = crc & 1;
            crc >>= 1;

            if(bit)
                crc ^= 0xA001;
        }
    }

    return crc;
}

uint32_t crc32(uint8_t *data, int length)
{
    uint32_t crc = 0xFFFFFFFF;

    for(int i = 0; i < length; i++)
    {
        crc ^= data[i];
        for(int j = 0; j < 8; j++)
        {
            bool bit = crc & 1;
            crc >>= 1;

            if(bit)
                crc ^= 0xEDB88320;
        }
    }

    return crc;
}

