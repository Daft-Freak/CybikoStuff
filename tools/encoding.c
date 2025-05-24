#include <stddef.h>

#include "encoding.h"

// bit stream for BMC
typedef struct BitStream
{
    uint8_t *data, *dataEnd;
    int numBits;
    uint8_t curByte;
} BitStream;

static void BitStream_init(BitStream *stream, uint8_t *data, int length)
{
    stream->data = data;
    stream->dataEnd = data + length;
    stream->numBits = 0;
}

static int BitStream_readBit(BitStream *stream)
{
    if(!stream->numBits)
    {
        if(stream->data == stream->dataEnd)
            return 0;

        stream->curByte = *stream->data++;
        stream->numBits = 8;
    }

    int ret = stream->curByte & 1;
    stream->curByte >>= 1;

    stream->numBits--;

    return ret;
}

static int BitStream_readBits(BitStream *stream, int bits)
{
    int ret = 0;

    for(int i = 0; i < bits; i++)
        ret = (ret << 1) | BitStream_readBit(stream);

    return ret;
}

static void BitStream_writeBit(BitStream *stream, int bit)
{
    if(stream->data == stream->dataEnd)
        return;

    stream->curByte >>= 1;
    stream->curByte |= bit << 7;

    stream->numBits++;

    if(stream->numBits == 8)
    {
        *stream->data++ = stream->curByte;
        stream->numBits = 0;
    }
}

static int BitStream_writeBits(BitStream *stream, int bits, int value)
{
    int ret = 0;

    for(int i = 0; i < bits; i++, value <<= 1)
        BitStream_writeBit(stream, (value >> (bits - 1)) & 1);

    return ret;
}

static void BitStream_flush(BitStream *stream)
{
    if(stream->data == stream->dataEnd || stream->numBits == 0)
        return;

    stream->curByte >>= (8 - stream->numBits);

    *stream->data++ = stream->curByte;
    stream->numBits = 0;
}

static bool BitStream_eof(BitStream *stream)
{
    return stream->data == stream->dataEnd;
}

// encoding used by USB boot (also accepted by serial boot)
void encodeBoot(uint8_t *in, uint8_t *out, int length)
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

// "BMC" is similar to LZSS, but encoded as a stream of bits with the flags inline
// also, the offset/length fields have a variable-length encoding
static int readBMCLength(BitStream *stream)
{
    // range is 2-256

    int length;
    uint8_t tmp = BitStream_readBits(stream, 2);

    switch(tmp)
    {
        case 0: length = 2; break;
        case 1: length = 3; break;
        case 2: length = BitStream_readBits(stream, 1) + 4; break;
        case 3:
            tmp = BitStream_readBits(stream, 2);
            switch(tmp)
            {
                case 0: length = 6; break;
                case 1: length = BitStream_readBits(stream, 1) + 8; break;
                case 2:
                    if(BitStream_readBit(stream))
                        length = BitStream_readBits(stream, 1) + 10;
                    else
                        length = 7;
                    break;
                case 3:
                    tmp = BitStream_readBits(stream, 3);
                    switch(tmp)
                    {
                        case 0: length = 12; break;
                        case 1: length = 13; break;
                        case 2: length = BitStream_readBits(stream, 2) + 20; break;
                        case 3: length = BitStream_readBits(stream, 1) + 16; break;
                        case 4: length = BitStream_readBits(stream, 3) + 24; break;
                        case 5: length = 14; break;
                        case 6:
                            if(BitStream_readBit(stream))
                                length = BitStream_readBits(stream, 1) + 18;
                            else
                                length = BitStream_readBits(stream, 5) + 32;
                            break;
                        case 7:
                            if(BitStream_readBit(stream))
                            {
                                if(BitStream_readBit(stream))
                                {
                                    if(BitStream_readBit(stream))
                                        length = 256;
                                    else
                                        length = BitStream_readBits(stream, 7) + 128;
                                }
                                else
                                    length = BitStream_readBits(stream, 6) + 64;
                            }
                            else
                                length = 15;
                    }
            }
    }

    return length;
}

static int readBMCOffset(BitStream *stream)
{
    // range is 0-8191

    int offset;
    uint8_t tmp = BitStream_readBits(stream, 3);

    switch(tmp)
    {
        case 0: offset = BitStream_readBits(stream, 10) + 1024; break;
        case 1: offset = BitStream_readBits(stream, 9) + 512; break;
        case 2: offset = BitStream_readBits(stream, 8) + 256; break;
        case 3: offset = BitStream_readBits(stream, 6) + 64; break;
        case 4: offset = BitStream_readBits(stream, 7) + 128; break;
        case 5:
            if(BitStream_readBit(stream))
                offset = BitStream_readBits(stream, 12) + 4096;
            else
                offset = BitStream_readBits(stream, 11) + 2048;
            break;
        case 6:
            if(BitStream_readBit(stream))
                offset = BitStream_readBits(stream, 4) + 16;
            else
                offset = BitStream_readBits(stream, 5) + 32;
            break;
        case 7:
            tmp = BitStream_readBits(stream, 2);
            switch(tmp)
            {
                case 0: offset = BitStream_readBit(stream); break;
                case 1: offset = BitStream_readBits(stream, 2) + 6; break;
                case 2:
                    if(BitStream_readBit(stream))
                        offset = 2;
                    else
                        offset = BitStream_readBit(stream) + 4;
                    break;
                case 3:
                    tmp = BitStream_readBits(stream, 2);
                    switch(tmp)
                    {
                        case 0: offset = BitStream_readBit(stream) + 12; break;
                        case 1: offset = BitStream_readBit(stream) + 14; break;
                        case 2: offset = 3; break;
                        case 3: offset = BitStream_readBit(stream) + 10; break;
                    }
            }
    }

    return offset;
}

bool decodeBMC(uint8_t *in, uint8_t *out, int inLength, int outLength)
{
    BitStream inStream;
    BitStream_init(&inStream, in, inLength);

    uint8_t *outEnd = out + outLength;
    uint8_t *outPtr = out;

    while(!BitStream_eof(&inStream) && outPtr != outEnd)
    {
        // condition here is reversed compared to the LZSS impl
        if(BitStream_readBit(&inStream))
        {
            // read the length(variable length)
            int len = readBMCLength(&inStream);

            // ... and the offset
            int offset = readBMCOffset(&inStream);

            // len added to offset, so can't copy past end/repeat data
            uint8_t *copyPtr = outPtr - (offset + len);

            // some validation
            if(copyPtr < out || outPtr + len > outEnd)
                break;

            for(int i = 0; i < len; i++)
                *outPtr++ = *copyPtr++;
        }
        else
            *outPtr++ = BitStream_readBits(&inStream, 8);
    }

    // fail if partial
    return BitStream_eof(&inStream) && outPtr == outEnd;
}

static void writeBMCLength(BitStream *stream, int length)
{
    // range is 2-256

    if(length < 4) // 2 bits
        BitStream_writeBits(stream, 2, length - 2);
    else if(length < 6) // 3 bits
    {
        BitStream_writeBits(stream, 2, 2);
        BitStream_writeBit(stream, length - 4);
    }
    else
    {
        BitStream_writeBits(stream, 2, 3);

        if(length == 6) // 4 bits
            BitStream_writeBits(stream, 2, 0);
        else if(length == 7) // 5 bits
        {
            BitStream_writeBits(stream, 2, 2);
            BitStream_writeBit(stream, 0);
        }
        else if(length < 10) // 5 bits
        {
            BitStream_writeBits(stream, 2, 1);
            BitStream_writeBit(stream, length - 8);
        }
        else if(length < 12) // 6 bits
        {
            BitStream_writeBits(stream, 2, 2);
            BitStream_writeBit(stream, 1);
            BitStream_writeBit(stream, length - 10);
        }
        else
        {
            BitStream_writeBits(stream, 2, 3);

            if(length < 14) // 7 bits
                BitStream_writeBits(stream, 3, length - 12);
            else if(length == 14) // 7 bits
                BitStream_writeBits(stream, 3, 5);
            else if(length == 15) // 8 bits
            {
                BitStream_writeBits(stream, 3, 7);
                BitStream_writeBit(stream, 0);
            }
            else if(length < 18) // 8 bits
            {
                BitStream_writeBits(stream, 3, 3);
                BitStream_writeBit(stream, length - 16);
            }
            else if(length < 20) // 9 bits
            {
                BitStream_writeBits(stream, 3, 6);
                BitStream_writeBit(stream, 1);
                BitStream_writeBit(stream, length - 18);
            }
            else if(length < 24) // 9 bits
            {
                BitStream_writeBits(stream, 3, 2);
                BitStream_writeBits(stream, 2, length - 20);
            }
            else if(length < 32) // 10 bits
            {
                BitStream_writeBits(stream, 3, 4);
                BitStream_writeBits(stream, 3, length - 24);
            }
            else if(length < 64) // 13 bits
            {
                BitStream_writeBits(stream, 3, 6);
                BitStream_writeBit(stream, 0);
                BitStream_writeBits(stream, 5, length - 32);
            }
            else if(length < 128) // 15 bits
            {
                BitStream_writeBits(stream, 3, 7);
                BitStream_writeBit(stream, 1);
                BitStream_writeBit(stream, 0);
                BitStream_writeBits(stream, 6, length - 64);
            }
            else if(length < 256) // 17 bits
            {
                BitStream_writeBits(stream, 3, 7);
                BitStream_writeBit(stream, 1);
                BitStream_writeBit(stream, 1);
                BitStream_writeBit(stream, 0);
                BitStream_writeBits(stream, 7, length - 128);
            }
            else if(length == 256) // 10 bits
            {
                BitStream_writeBits(stream, 3, 7);
                BitStream_writeBit(stream, 1);
                BitStream_writeBit(stream, 1);
                BitStream_writeBit(stream, 1);
            }
        }
    }
}

static void writeBMCOffset(BitStream *stream, int offset)
{
    // range is 0-8191

    if(offset < 2) // 6 bits
    {
        BitStream_writeBits(stream, 3, 7);
        BitStream_writeBits(stream, 2, 0);
        BitStream_writeBit(stream, offset);
    }
    else if(offset == 2) // 6 bits
    {
        BitStream_writeBits(stream, 3, 7);
        BitStream_writeBits(stream, 2, 2);
        BitStream_writeBit(stream, 1);
    }
    else if(offset == 3) // 7 bits
    {
        BitStream_writeBits(stream, 3, 7);
        BitStream_writeBits(stream, 2, 3);
        BitStream_writeBits(stream, 2, 2);
    }
    else if(offset < 6) // 7 bits
    {
        BitStream_writeBits(stream, 3, 7);
        BitStream_writeBits(stream, 2, 2);
        BitStream_writeBit(stream, 0);
        BitStream_writeBit(stream, offset - 4);
    }
    else if(offset < 10) // 7 bits
    {
        BitStream_writeBits(stream, 3, 7);
        BitStream_writeBits(stream, 2, 1);
        BitStream_writeBits(stream, 2, offset - 6);
    }
    else if(offset < 12) // 8 bits
    {
        BitStream_writeBits(stream, 3, 7);
        BitStream_writeBits(stream, 2, 3);
        BitStream_writeBits(stream, 2, 3);
        BitStream_writeBit(stream, offset - 10);
    }
    else if(offset < 14) // 8 bits
    {
        BitStream_writeBits(stream, 3, 7);
        BitStream_writeBits(stream, 2, 3);
        BitStream_writeBits(stream, 2, 0);
        BitStream_writeBit(stream, offset - 12);
    }
    else if(offset < 16) // 8 bits
    {
        BitStream_writeBits(stream, 3, 7);
        BitStream_writeBits(stream, 2, 3);
        BitStream_writeBits(stream, 2, 1);
        BitStream_writeBit(stream, offset - 14);
    }
    else if(offset < 32) // 8 bits
    {
        BitStream_writeBits(stream, 3, 6);
        BitStream_writeBit(stream, 1);
        BitStream_writeBits(stream, 4, offset - 16);
    }
    else if(offset < 64) // 9 bits
    {
        BitStream_writeBits(stream, 3, 6);
        BitStream_writeBit(stream, 0);
        BitStream_writeBits(stream, 5, offset - 32);
    }
    else if(offset < 128) // 9 bits
    {
        BitStream_writeBits(stream, 3, 3);
        BitStream_writeBits(stream, 6, offset - 64);
    }
    else if(offset < 256) // 10 bits
    {
        BitStream_writeBits(stream, 3, 4);
        BitStream_writeBits(stream, 7, offset - 128);
    }
    else if(offset < 512) // 11 bits
    {
        BitStream_writeBits(stream, 3, 2);
        BitStream_writeBits(stream, 8, offset - 256);
    }
    else if(offset < 1024) // 12 bits
    {
        BitStream_writeBits(stream, 3, 1);
        BitStream_writeBits(stream, 9, offset - 512);
    }
    else if(offset < 2048) // 13 bits
    {
        BitStream_writeBits(stream, 3, 0);
        BitStream_writeBits(stream, 10, offset - 1024);
    }
    else if(offset < 4096) // 15 bits
    {
        BitStream_writeBits(stream, 3, 5);
        BitStream_writeBit(stream, 0);
        BitStream_writeBits(stream, 11, offset - 2048);
    }
    else if(offset < 8192) // 16 bits
    {
        BitStream_writeBits(stream, 3, 5);
        BitStream_writeBit(stream, 1);
        BitStream_writeBits(stream, 12, offset - 4096);
    }
}

int encodeBMC(uint8_t *in, uint8_t *out, int inLength, int maxOutLength)
{
    BitStream outStream;
    BitStream_init(&outStream, out, maxOutLength);

    uint8_t *inEnd = in + inLength;

    uint8_t *inPtr = in;

    while(inPtr != inEnd)
    {
        int bestLen = 0;
        int bestOff = 0;

        // search for longest match
        for(int offset = 1; offset <= 0x8192; offset++)
        {
            uint8_t *ptr = inPtr - offset;
            if(ptr < in)
                break;

            int len = 0;
            for(; len <= 256 && inPtr + len != inEnd && ptr != inPtr; len++)
            {
                if(ptr[len] != inPtr[len])
                    break;
            }

            // clamp
            if(len > 256 || ptr == inPtr)
                len--;

            if(len > bestLen && offset >= len)
            {
                bestLen = len;
                bestOff = offset;
            }
        }
        if(bestLen >= 2)
        {
            BitStream_writeBit(&outStream, 1);
            writeBMCLength(&outStream, bestLen);
            writeBMCOffset(&outStream, bestOff - bestLen);

            inPtr += bestLen;
        }
        else
        {
            // no match or too short, copy
            BitStream_writeBit(&outStream, 0);
            BitStream_writeBits(&outStream, 8, *inPtr++);
        }
    }

    BitStream_flush(&outStream);

    return outStream.data - out;
}
