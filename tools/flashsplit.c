#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "encoding.h"

static uint32_t readInt(FILE *file)
{
    uint8_t bytes[4];
    fread(bytes, 4, 1, file);

    return (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
}

static uint8_t *readChunk(FILE *inFile, int offset, int size)
{
    uint8_t *data = malloc(size);
    fseek(inFile, offset, SEEK_SET);
    fread(data, 1, size, inFile);

    return data;
}

static bool writeFile(const char *outDir, const char *outName, uint8_t *data, int size)
{
    char path[1024];

    char dirEnd = outDir[strlen(outDir) - 1];

    snprintf(path, sizeof(path), "%s%s%s", outDir, dirEnd == '/' ? "" : "/", outName);

    printf("writing %s (size %i)\n", path, size);

    // open output
    FILE *outFile = fopen(path, "wb");
    if(!outFile)
    {
        fprintf(stderr, "Failed to open %s\n", path);
        return false;
    }
    // write
    fwrite(data, 1, size, outFile);

    fclose(outFile);
    return true;
}

static bool extractChunk(FILE *inFile, const char *outDir, const char *outName, int offset, int size)
{
    uint8_t *data = readChunk(inFile, offset, size);

    bool ret = writeFile(outDir, outName, data, size);

    free(data);

    return ret;
}

static uint8_t *decompressOS(FILE *inFile, int offset)
{
    // we already checked the header, skip it
    fseek(inFile, offset + 4, SEEK_SET);

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
        return NULL;
    }

    free(inData);

    return outData;
}

static void usage()
{
    printf("usage: flashsplit [options] [input file] [output dir]\n\n");
}

int main(int argc, char *argv[])
{
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
                default:
                    printf("Invalid flag: %c\n", argv[i][j]);
                    usage();
                    return 1;
            }
        }
    }

    // check for in filenames
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

    fseek(inFile, 0, SEEK_END);
    long inLength = ftell(inFile);
    fseek(inFile, 0, SEEK_SET);

    const char *outDir = argv[i + 1];

    // there are four parts to the flash image:
    // - the second stage loader
    // - the compressed OS image
    // - the initial files for the filesystem
    // - a 0x800 byte config block at the end of the image

    // first, try to find the start of the OS image
    // assume it's aligned to 4 bytes
    int osStart = 0, osLen = 0, osDecompLen = 0;

    int searchEnd = 8192;
    uint8_t lzssHead[4] = {0x01, 0xC0, 0xFF, 0xAB};

    for(int i = 0; i < searchEnd; i += 4)
    {
        uint8_t buf[4];
        fseek(inFile, i, SEEK_SET);
        fread(buf, 4, 1, inFile);

        if(memcmp(buf, lzssHead, 4) == 0)
        {
            int compLen = readInt(inFile);
            int decompLen = readInt(inFile);

            // make sure sizes are sensible
            if(decompLen < compLen || decompLen > 1 * 1024 * 1024)
                continue;

            osStart = i;
            osLen = compLen + 12;
            osDecompLen = decompLen;
        }
    }

    if(!osStart)
    {
        fprintf(stderr, "Couldn't find OS image!\n");
        fclose(inFile);
        return 1;
    }

    printf("loader len %i, os image len %i\n", osStart, osLen);

    int res = 0;

    if(!extractChunk(inFile, outDir, "loader.bin", 0, osStart))
        res = 1;
    if(!extractChunk(inFile, outDir, "os.bin", osStart, osLen))
        res = 1;

    if(!extractChunk(inFile, outDir, "config.bin", inLength - 0x800, 0x800))
        res = 1;

    // read os again and decompress
    uint8_t *decompressedOS = decompressOS(inFile, osStart);
    int fileTableOffset = -1;
    if(decompressedOS)
    {
        if(!writeFile(outDir, "os_decompressed.bin", decompressedOS, osDecompLen))
            res = 1;

        // try to find flash file table
        // these are the names of the first two files
        const char *findStr = "System.a";
        const char *findStr2 = "autostart.cmd";
        int findLen = strlen(findStr);
        int findLen2 = strlen(findStr2);

        uint8_t *end = decompressedOS + osDecompLen;

        for(uint8_t *ptr = decompressedOS; ptr != end; ptr++)
        {
            if(*ptr != findStr[0] || ptr + findLen >= end)
                continue;

            // look for first string
            bool found = true;
            for(int i = 0; i < findLen && found; i++)
                found = ptr[i] == findStr[i];

            if(!found || ptr[findLen])
                continue;

            // look for second string
            uint8_t *ptr2 = ptr + findLen + 1;

            found = true;
            for(int i = 0; i < findLen2 && found; i++)
                found = ptr2[i] == findStr2[i];

            if(!found || ptr2[findLen2])
                continue;

            // now we need to find pointers to these strings
            uint32_t offset = ptr - decompressedOS;
            uint32_t addr = 0x480000 | offset;
            uint32_t addr2 = 0x480000 | (offset + findLen + 1);

            for(uint8_t *ptr2 = decompressedOS; ptr2 != end - 15; ptr2++)
            {
                if(ptr2[ 0] ==   addr  >> 24          &&
                   ptr2[ 1] == ((addr  >> 16) & 0xFF) &&
                   ptr2[ 2] == ((addr  >>  8) & 0xFF) &&
                   ptr2[ 3] == ( addr         & 0xFF) &&
                   ptr2[12] ==   addr2 >> 24          &&
                   ptr2[13] == ((addr2 >> 16) & 0xFF) &&
                   ptr2[14] == ((addr2 >>  8) & 0xFF) &&
                   ptr2[15] == ( addr2        & 0xFF))
                {
                    fileTableOffset = ptr2 - decompressedOS;
                    break;
                }
            }

            // done
            if(fileTableOffset != -1)
                break;
        }
    }
    else
    {
        fprintf(stderr, "Failed to decompress OS\n");
        res = 1;
    }

    // extract the files
    if(decompressedOS && fileTableOffset)
    {
        printf("found file list at %x\n", fileTableOffset);

        uint8_t *ptr = decompressedOS + fileTableOffset;

        while(true)
        {
            // not much point in rading the upper bits
            uint32_t nameAddr = ptr[1] << 16 | ptr[2] << 8 | ptr[3];
            nameAddr &= 0x3FFFF;
            if(!nameAddr)
                break;

            uint32_t offset = ptr[5] << 16 | ptr[6] << 8 | ptr[7];
            uint32_t size = ptr[9] << 16 | ptr[10] << 8 | ptr[11];

            char *name = (char *)decompressedOS + nameAddr;

            if(!extractChunk(inFile, outDir, name, osStart + osLen + offset, size))
                res = 1;

            ptr += 12;
        }
    }
    else
    {
        // dump the whole block, including padding...
        int remainingLen = inLength - (osStart + osLen + 0x800);
        if(!extractChunk(inFile, outDir, "files.bin", osStart + osLen, remainingLen))
            res = 1;
    }

    if(decompressedOS)
        free(decompressedOS);

    fclose(inFile);

    return res;
}
