#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "encoding.h"

typedef struct ArchiveEntry
{
    const char *name;
    uint32_t offset;
    uint32_t size;
} ArchiveEntry;

typedef struct Archive
{
    FILE *file;
    int numEntries;
    ArchiveEntry *entries;
} Archive;

static uint32_t readShort(FILE *file)
{
    uint8_t bytes[2];
    fread(bytes, 2, 1, file);

    return (bytes[0] << 8) | bytes[1];
}

static void usage()
{
    printf("usage: archive [options] [input file]\n\n");
}

bool Archive_init(Archive *archive, FILE *file)
{
    archive->file = file;

    // "Cy" header
    uint8_t buf[2];
    if(fread(buf, 1, 2, file) != 2 || buf[0] != 'C' || buf[1] != 'y')
        return false;

    archive->numEntries = readShort(file);
    archive->entries = calloc(archive->numEntries, sizeof(ArchiveEntry));

    if(!archive->entries)
    {
        archive->numEntries = 0;
        return false;
    }

    int indexSize = readShort(file);

    uint8_t *index = malloc(indexSize);

    fread(index, 1, indexSize, file);

    uint8_t *ptr = index;

    for(int i = 0; i < archive->numEntries; i++, ptr += 10)
    {
        int nameOffset = ptr[0] << 8 | ptr[1];
        archive->entries[i].name = strdup((char *)index + nameOffset - 6);
        archive->entries[i].offset = ptr[2] << 24 | ptr[3] << 16 | ptr[4] << 8 | ptr[5];
        archive->entries[i].size = ptr[6] << 24 | ptr[7] << 16 | ptr[8] << 8 | ptr[9];
    }

    free(index);

    return true;
}

uint32_t Archive_getDecompressedSize(Archive *archive, int entryIndex)
{
    ArchiveEntry *entry = archive->entries + entryIndex;

    fseek(archive->file, entry->offset, SEEK_SET);

    int compression = fgetc(archive->file);

    if(compression == 0)
    {
        // uncompressed
        return entry->size;
    }
    else if(compression == 2)
    {
        uint8_t bytes[4];
        fread(bytes, 4, 1, archive->file);

        return (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
    }

    return 0;
    
}

void Archive_free(Archive *archive)
{
    for(int i = 0; i < archive->numEntries; i++)
        free((char *)archive->entries[i].name);
    free(archive->entries);
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
    if(argc - i != 1)
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

    Archive archive;

    if(!Archive_init(&archive, inFile))
    {
        fprintf(stderr, "Failed to read index\n");
        return 1;
    }

    int res = 0;

    // list entries
    for(int i = 0; i < archive.numEntries; i++)
    {
        ArchiveEntry *entry = archive.entries + i;
        uint32_t decompressedSize = Archive_getDecompressedSize(&archive, i);
        printf("entry %2i size %6i (decompressed %6i) offset %6i name %s\n", i, entry->size, decompressedSize, entry->offset, entry->name);
    }

    Archive_free(&archive);

    fclose(inFile);

    return res;
}
