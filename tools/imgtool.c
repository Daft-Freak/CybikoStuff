#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "encoding.h"

typedef enum Encoding
{
    Encoding_None,
    Encoding_Boot, // this is the format USB boot expects, but isn't named in the serial output
    Encoding_LZSS
} Encoding;

static size_t writeInt(uint32_t i, FILE *file)
{
    uint8_t bytes[4];

    bytes[0] = i >> 24;
    bytes[1] = i >> 16;
    bytes[2] = i >> 8;
    bytes[3] = i;

    return fwrite(bytes, 4, 1, file);
}

static uint32_t readInt(FILE *file)
{
    uint8_t bytes[4];
    fread(bytes, 4, 1, file);

    return (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
}

static bool doDecodeBoot(FILE *inFile, FILE *outFile)
{
    uint8_t head[4];
    uint8_t expectedHead[4] = {0xFF, 0xC0, 0xFF, 0xAB};
    fread(head, 4, 1, inFile);

    if(memcmp(head, expectedHead, 4) != 0)
    {
        printf("Invalid boot header! (expected FFC0FFAB, got %02X%02X%02X%02X)\n", head[0], head[1], head[2], head[3]);
        return false;
    }

    uint32_t length = readInt(inFile);
    fseek(inFile, 4, SEEK_CUR); // out length, should be the same

    // read in file
    uint8_t *inData = malloc(length), *outData = malloc(length);
    fread(inData, 1, length, inFile);

    // decode
    encodeBoot(inData, outData, length);

    fwrite(outData, 1, length, outFile);

    free(inData);
    free(outData);

    return true;
}

static bool doEncodeBoot(FILE *inFile, FILE *outFile)
{
    uint8_t head[4] = {0xFF, 0xC0, 0xFF, 0xAB};
    fwrite(head, 4, 1, outFile);

    // read in file
    fseek(inFile, 0, SEEK_END);
    long length = ftell(inFile);
    fseek(inFile, 0, SEEK_SET);

    uint8_t *inData = malloc(length), *outData = malloc(length);
    fread(inData, 1, length, inFile);

    // write lengths
    writeInt(length, outFile);
    writeInt(length, outFile);

    // encode
    encodeBoot(inData, outData, length);

    fwrite(outData, 1, length, outFile);

    free(inData);
    free(outData);

    return true;
}

static bool doDecodeLZSS(FILE *inFile, FILE *outFile)
{
    uint8_t head[4];
    uint8_t expectedHead[4] = {0x01, 0xC0, 0xFF, 0xAB};
    fread(head, 4, 1, inFile);

    if(memcmp(head, expectedHead, 4) != 0)
    {
        printf("Invalid LZSS header! (expected 01C0FFAB, got %02X%02X%02X%02X)\n", head[0], head[1], head[2], head[3]);
        return false;
    }

    uint32_t compressedLength = readInt(inFile);
    uint32_t decompressedLength = readInt(inFile);

    // read in file
    uint8_t *inData = malloc(compressedLength), *outData = malloc(decompressedLength);
    fread(inData, 1, compressedLength, inFile);

    // decode
    if(!decodeLZSS(inData, outData, compressedLength, decompressedLength))
    {
        free(inData);
        free(outData);
        return false;
    }

    fwrite(outData, 1, decompressedLength, outFile);

    free(inData);
    free(outData);

    return true;
}

static bool doEncodeLZSS(FILE *inFile, FILE *outFile)
{
    // header
    uint8_t head[4] = {0x01, 0xC0, 0xFF, 0xAB};
    fwrite(head, 4, 1, outFile);

    // read in file
    fseek(inFile, 0, SEEK_END);
    long inLength = ftell(inFile);
    fseek(inFile, 0, SEEK_SET);

    uint8_t *inData = malloc(inLength);
    fread(inData, 1, inLength, inFile);

    // worst case output buffer
    uint8_t *outData = malloc(inLength + (inLength + 7) / 8);
    int outLength = 0;

    // encode
    outLength = encodeLZSS(inData, outData, inLength);

    // write lengths and data
    writeInt(outLength, outFile);
    writeInt(inLength, outFile);
    fwrite(outData, 1, outLength, outFile);

    free(inData);
    free(outData);

    return true;
}

static void usage()
{
    printf("usage: imgtool [options] [input file] [output file]\n\n");
    printf("\t-d: decode file\n");
    printf("\t-e: encode file (default)\n");
    printf("\t-b: \"boot\" encoding\n");
    printf("\t-l: LZSS endoding\n");
    printf("\t-o [offset]: seek to offset in input file before decode\n");
}

int main(int argc, char *argv[])
{
    Encoding enc = Encoding_None;
    bool decode = false;
    int inOffset = 0;

    // parse arguments
    int i = 1;
    for(; i < argc; i++)
    {
        if(argv[i][0] != '-')
            break;

        const char *flags = argv[i];

        for(int j = 1; flags[j]; j++)
        {
            switch(flags[j])
            {
                case 'd':
                    decode = true;
                    break;
                case 'e':
                    decode = false;
                    break;
                case 'b':
                    enc = Encoding_Boot;
                    break;
                case 'l':
                    enc = Encoding_LZSS;
                    break;
                case 'o':
                    // not last arg, no more flags
                    if(i + 1 < argc && !flags[j + 1])
                    {
                        inOffset = atoi(argv[++i]);
                    }
                    else
                    {
                        printf("Invalid flag: %c\n", argv[i][j]);
                        usage();
                        return 1;
                    }
                    break;

                default:
                    printf("Invalid flag: %c\n", argv[i][j]);
                    usage();
                    return 1;
            }
        }
    }

    if(enc == Encoding_None)
    {
        printf("No encoding specified\n");
        usage();
        return 1;
    }

    // check for in/out filenames
    if(argc - i != 2)
    {
        usage();
        return 1;
    }

    FILE *inFile = fopen(argv[i], "rb");
    if(!inFile)
    {
        fprintf(stderr, "Failed to open input file\n");
        return 1;
    }

    fseek(inFile, inOffset, SEEK_SET);

    FILE *outFile = fopen(argv[i + 1], "wb");
    if(!outFile)
    {
        fclose(inFile);
        fprintf(stderr, "Failed to open output file\n");
        return 1;
    }

    int res = 0;

    if(enc == Encoding_Boot)
    {
        if(decode)
            res = doDecodeBoot(inFile, outFile) ? 0 : 1;
        else
            res = doEncodeBoot(inFile, outFile) ? 0 : 1;
    }
    else if(enc == Encoding_LZSS)
    {
        if(decode)
            res = doDecodeLZSS(inFile, outFile) ? 0 : 1;
        else
            res = doEncodeLZSS(inFile, outFile) ? 0 : 1;
    }
    fclose(inFile);
    fclose(outFile);

    return res;
}
