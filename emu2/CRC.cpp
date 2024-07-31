#include "CRC.h"

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

uint16_t fsChecksum(uint8_t *data, int length)
{
    uint16_t ret = 0;

    for(int i = 0; i < length; i++)
    {
        ret = ret ^ data[i] ^ i;
        ret = ret << 1 | ret >> 15;
    }

    return ret;
}
