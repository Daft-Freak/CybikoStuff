#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "encoding.h"

typedef enum Action
{
    Action_None,
    Action_List,
    Action_Extract,
    Action_ExecutableInfo,
} Action;

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
        return entry->size - 1;
    }
    else if(compression == 2)
    {
        uint8_t bytes[4];
        fread(bytes, 4, 1, archive->file);

        return (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
    }

    return 0;
}

uint8_t *Archive_readEntry(Archive *archive, int entryIndex)
{
    ArchiveEntry *entry = archive->entries + entryIndex;

    fseek(archive->file, entry->offset, SEEK_SET);

    int compression = fgetc(archive->file);

    uint8_t *ret = NULL;

    if(compression == 0)
    {
        // uncompressed
        ret = malloc(entry->size - 1);
        fread(ret, 1, entry->size - 1, archive->file);
    }
    // 1 would probably be LZSS if it was supported...
    else if(compression == 2)
    {
        uint8_t bytes[4];
        fread(bytes, 4, 1, archive->file);

        uint32_t outSize = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
        uint32_t inSize = entry->size - 5;

        // read in file
        uint8_t *inData = malloc(inSize), *outData = malloc(outSize);
        fread(inData, 1, inSize, archive->file);

        // decode
        if(decodeBMC(inData, outData, inSize, outSize))
            ret = outData;
        else
            free(outData);

        free(inData);
    }

    return ret;
}

void Archive_free(Archive *archive)
{
    for(int i = 0; i < archive->numEntries; i++)
        free((char *)archive->entries[i].name);
    free(archive->entries);
}

static bool printExecutableInfo(uint8_t *exeData)
{
    uint16_t magic1 = exeData[0] << 8 | exeData[1];
    uint16_t magic2 = exeData[2] << 8 | exeData[3];
    exeData += 4;

    // not sure the first one is actually checked, but the second one is
    if(magic1 != 0x91AF || magic2 < 2)
    {
        printf("expected 91AF 0002 got %04X %04X\n", magic1, magic2);
        return false;
    }

    uint32_t imageSize = exeData[0] << 24 | exeData[1] << 16 | exeData[2] << 8 | exeData[3];
    exeData += 4;

    uint32_t extraSize = exeData[0] << 24 | exeData[1] << 16 | exeData[2] << 8 | exeData[3];
    exeData += 4;

    uint32_t exportsSize = exeData[0] << 24 | exeData[1] << 16 | exeData[2] << 8 | exeData[3];
    exeData += 4;

    uint32_t importsSize = exeData[0] << 24 | exeData[1] << 16 | exeData[2] << 8 | exeData[3];
    exeData += 4;

    uint32_t relocsSize = exeData[0] << 24 | exeData[1] << 16 | exeData[2] << 8 | exeData[3];
    exeData += 4;

    uint32_t entryOffset = exeData[0] << 24 | exeData[1] << 16 | exeData[2] << 8 | exeData[3];
    exeData += 4;

    uint32_t ctorsOffset = exeData[0] << 24 | exeData[1] << 16 | exeData[2] << 8 | exeData[3];
    exeData += 4;

    uint32_t ctorsSize = exeData[0] << 24 | exeData[1] << 16 | exeData[2] << 8 | exeData[3];
    exeData += 4;

    uint32_t dtorsOffset = exeData[0] << 24 | exeData[1] << 16 | exeData[2] << 8 | exeData[3];
    exeData += 4;

    uint32_t dtorsSize = exeData[0] << 24 | exeData[1] << 16 | exeData[2] << 8 | exeData[3];
    exeData += 4;

    // parse imports
    char *importsData = (char *)exeData + imageSize;
    char *importsEnd = importsData + importsSize;

    int numImports = 0;
    for(char *p = importsData; p != importsEnd; p += strlen(p) + 1)
        numImports++;

    char **imports = malloc(numImports * sizeof(char *));
    int i = 0;
    for(char *p = importsData; p != importsEnd; p += strlen(p) + 1, i++)
    {
        imports[i] = p;
        // rename ordinals to OS
        if(strcmp(imports[i], "ordinals") == 0)
            imports[i] = "OS";
    }

    printf("Image size %i bytes (+%i)\n", imageSize, extraSize);
    printf("Exports %i symbols\n", exportsSize / 4);
    printf("Imports from %i modules\n", numImports);

    for(int i = 0; i < numImports; i++)
        printf("\t%s\n", imports[i]);

    printf("Entry at offset %x, %i ctors at %x, %i dtors at %x\n", entryOffset, ctorsSize / 4, ctorsOffset, dtorsSize / 4, dtorsOffset);

    printf("%i relocations\n", relocsSize / 4);
    uint8_t *image = exeData;
    uint8_t *relocs = exeData + imageSize + importsSize;
    uint8_t *relocsEnd = relocs + relocsSize;

    for(uint8_t *p = relocs; p != relocsEnd; p += 4)
    {
        uint32_t reloc = p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];
        uint32_t offset = reloc & 0xFFFFFF;

        // import
        if(reloc & 0x80000000)
        {
            uint32_t code = image[offset] << 24 | image[offset + 1] << 16 | image[offset + 2] << 8 | image[offset + 3];
            int index = code & 0x1FFF;
            int moduleIndex = (reloc >> 24) & 0x7F;
            printf("\t%s index %i at %06X\n", imports[moduleIndex], index, offset);
        }
        else // self
            printf("\tself at %06X\n", offset);
    }

    printf("\n");

    free(imports);

    return true;
}

static void usage()
{
    printf("usage: archive [options] [input file]\n\n");
    printf("\t-l: list entries\n");
    printf("\t-e: extract entries\n");
    printf("\t-i: display info about executables\n");
}

int main(int argc, char *argv[])
{
    Action action = Action_None;

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
                case 'l':
                    action = Action_List;
                    break;
                case 'e':
                    action = Action_Extract;
                    break;
                case 'i': // 'e' was taken
                    action = Action_ExecutableInfo;
                    break;

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

    switch(action)
    {
        case Action_List:
            // list entries
            for(int i = 0; i < archive.numEntries; i++)
            {
                ArchiveEntry *entry = archive.entries + i;
                uint32_t decompressedSize = Archive_getDecompressedSize(&archive, i);
                printf("entry %2i size %6i (decompressed %6i) offset %6i name %s\n", i, entry->size, decompressedSize, entry->offset, entry->name);
            }
            break;

        case Action_Extract:
            // extract entries
            for(int i = 0; i < archive.numEntries; i++)
            {
                ArchiveEntry *entry = archive.entries + i;
                uint8_t *data = Archive_readEntry(&archive, i);

                if(data)
                {
                    FILE *outFile = fopen(entry->name, "wb");
                    if(outFile)
                    {
                        printf("writing %s\n", entry->name);
                        fwrite(data, 1, Archive_getDecompressedSize(&archive, i), outFile);
                        fclose(outFile);
                    }
                    else
                        fprintf(stderr, "Failed to open %s!\n", entry->name);
                    free(data);
                }
                else
                {
                    fprintf(stderr, "Failed to extract %s!\n", entry->name);
                    res = 1;
                }
            }
            break;

        case Action_ExecutableInfo:
            // extract entries
            for(int i = 0; i < archive.numEntries; i++)
            {
                ArchiveEntry *entry = archive.entries + i;
                int nameLen = strlen(entry->name);
                bool isExecutable = nameLen > 2 && entry->name[nameLen - 2] == '.' && entry->name[nameLen - 1] == 'e';

                if(isExecutable)
                {
                    printf("%s:\n", entry->name);
                    uint8_t *data = Archive_readEntry(&archive, i);
                    if(data)
                    {
                        if(!printExecutableInfo(data))
                            res = 1;
                        free(data);
                    }
                    else
                        res = 1;
                }
            }
            break;

        default:
            printf("Nothing to do!\n");
    }


    Archive_free(&archive);

    fclose(inFile);

    return res;
}
