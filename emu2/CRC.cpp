#include "CRC.h"

// from usbcon.c
static uint32_t crc32Table[256] = {0};

void initCRCTables()
{
    for(int i = 0; i < 256; i++)
    {
        uint32_t crc = i;
        for(int j = 0; j < 8; j++)
        {
            bool bit = crc & 1;
            crc >>= 1;

            if(bit)
                crc ^= 0xEDB88320;
        }

        crc32Table[i] = crc;
    }
}

uint32_t crc32(uint8_t *data, int length)
{
    uint32_t crc = 0xFFFFFFFF;

    for(int i = 0; i < length; i++)
    {
        uint8_t v = (crc & 0xFF) ^ data[i];
        crc = (crc >> 8) ^ crc32Table[v];
    }

    return crc;
}
