#include <stddef.h>

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

int encodeLZSS(uint8_t *in, uint8_t *out, int inLength)
{
     uint8_t *inEnd = in + inLength;

    uint8_t *inPtr = in;
    uint8_t *outPtr = out;

    uint8_t *flagsPtr = NULL;
    int flagBitsUsed = 8;
    
    while(inPtr != inEnd)
    {
        if(flagBitsUsed == 8)
        {
            flagsPtr = outPtr++;
            *flagsPtr = 0;
            flagBitsUsed = 0;
        }

        int bestLen = 0;
        int bestOff = 0;

        // search for longest match
        for(int offset = 1; offset <= 0xFFF; offset++)
        {
            uint8_t *ptr = inPtr - offset;
            if(ptr < in)
                break;

            int len = 0;
            for(; len < 16 + 3 && inPtr + len != inEnd; len++)
            {
                if(ptr[len] != inPtr[len])
                    break;
            }

            // clamp
            if(len == 16 + 3)
                len--;

            if(len > bestLen)
            {
                bestLen = len;
                bestOff = offset;
            }
        }
        if(bestLen >= 3)
        {
            *outPtr++ = bestOff & 0xFF;
            *outPtr++ = (bestLen - 3) | (bestOff & 0xF00) >> 4;
            inPtr += bestLen;
        }
        else
        {
            // no match or too short, copy
            *flagsPtr |= 1 << flagBitsUsed;
            *outPtr++ = *inPtr++;
        }
        flagBitsUsed++;
    }

    return outPtr - out;
}