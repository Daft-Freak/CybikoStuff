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

bool decodeLZSS(uint8_t *in, uint8_t *out, int inLength, int outLength)
{
    uint8_t *inEnd = in + inLength;
    uint8_t *outEnd = out + outLength;

    uint8_t *inPtr = in;
    uint8_t *outPtr = out;

    uint8_t flagBits;
    int flagBitsUsed = 8;

    while(inPtr != inEnd && outPtr != outEnd)
    {
        if(flagBitsUsed == 8)
        {
            // get more flag bits
            flagBits = *inPtr++;
            flagBitsUsed = 0;
        }

        if(flagBits & 1)
            *outPtr++ = *inPtr++;
        else
        {
            int offset = inPtr[0] | (inPtr[1] & 0xF0) << 4;
            int len = (inPtr[1] & 0xF) + 3;
            inPtr += 2;

            uint8_t *copyPtr = outPtr - offset;

            // some validation
            if(copyPtr < out || outPtr + len > outEnd)
                break;

            for(int i = 0; i < len; i++)
                *outPtr++ = *copyPtr++;
        }

        flagBitsUsed++;
        flagBits >>= 1;
    }

    // fail if partial
    return inPtr == inEnd && outPtr == outEnd;
}
