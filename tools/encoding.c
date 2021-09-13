#include "encoding.h"

void encodeBMC(uint8_t *in, uint8_t *out, int length)
{
    uint32_t key = 0x1649b442;

    for(int i = 0; i < length; i++)
    {
        key = key * 0x343fd + 0x269ec3;
        uint8_t mask = key >> 16;
        out[i] = in[i] ^ mask;
    }
}
