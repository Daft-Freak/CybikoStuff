#include <stdio.h>

#include "Registers.hpp"

#include "helpers.h"

void extu32Test()
{
    struct TestValue32
    {
        unsigned long a, result;
        unsigned char cc;
    };

    static const TestValue32 tests32[]
    {
        {0x00000000, 0x00000000, 0x84},
        {0x00007FFF, 0x00007FFF, 0x80},
        {0x00008000, 0x00008000, 0x80},
        {0x0000FFFF, 0x0000FFFF, 0x80},
        {0x01230000, 0x00000000, 0x84},
        {0x45677FFF, 0x00007FFF, 0x80},
        {0x89AB8000, 0x00008000, 0x80},
        {0xCDEFFFFF, 0x0000FFFF, 0x80},
    };
    static const int numTests32 = sizeof(tests32) / sizeof(tests32[0]);

    int i = 0;
    writeString("EXTU (32)");

    unsigned char cc;
    unsigned long result;

    for(auto &test : tests32)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "extu %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a)
            :
        );

        if(result != test.result || cc != test.cc)
            break;

        i++;
    }

    if(i < numTests32)
    {
        char buf[60];
        auto &test = tests32[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %08lX (%02X), got %08lX (%02X)\n", i + 1, test.result, test.cc, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");
}

void extu16Test()
{
    struct TestValue16
    {
        unsigned int a, result;
        unsigned char cc;
    };

    static const TestValue16 tests16[]
    {
        {0x0000, 0x0000, 0x84},
        {0x007F, 0x007F, 0x80},
        {0x0080, 0x0080, 0x80},
        {0x00FF, 0x00FF, 0x80},
        {0x1300, 0x0000, 0x84},
        {0x577F, 0x007F, 0x80},
        {0x9B80, 0x0080, 0x80},
        {0xDFFF, 0x00FF, 0x80},
    };
    static const int numTests16 = sizeof(tests16) / sizeof(tests16[0]);

    int i = 0;
    writeString("EXTU (16)");

    unsigned char cc;
    unsigned int result;

    for(auto &test : tests16)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "extu %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a)
            :
        );

        if(result != test.result || cc != test.cc)
            break;

        i++;
    }

    if(i < numTests16)
    {
        char buf[60];
        auto &test = tests16[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %04X (%02X), got %04X (%02X)\n", i + 1, test.result, test.cc, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");
}

void exts32Test()
{
    struct TestValue32
    {
        unsigned long a, result;
        unsigned char cc;
    };

    static const TestValue32 tests32[]
    {
        {0x00000000, 0x00000000, 0x84},
        {0x00007FFF, 0x00007FFF, 0x80},
        {0x00008000, 0xFFFF8000, 0x88},
        {0x0000FFFF, 0xFFFFFFFF, 0x88},
        {0x01230000, 0x00000000, 0x84},
        {0x45677FFF, 0x00007FFF, 0x80},
        {0x89AB8000, 0xFFFF8000, 0x88},
        {0xCDEFFFFF, 0xFFFFFFFF, 0x88},
    };
    static const int numTests32 = sizeof(tests32) / sizeof(tests32[0]);

    int i = 0;
    writeString("EXTS (32)");

    unsigned char cc;
    unsigned long result;

    for(auto &test : tests32)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "exts %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a)
            :
        );

        if(result != test.result || cc != test.cc)
            break;

        i++;
    }

    if(i < numTests32)
    {
        char buf[60];
        auto &test = tests32[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %08lX (%02X), got %08lX (%02X)\n", i + 1, test.result, test.cc, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");
}

void exts16Test()
{
    struct TestValue16
    {
        unsigned int a, result;
        unsigned char cc;
    };

    static const TestValue16 tests16[]
    {
        {0x0000, 0x0000, 0x84},
        {0x007F, 0x007F, 0x80},
        {0x0080, 0xFF80, 0x88},
        {0x00FF, 0xFFFF, 0x88},
        {0x1300, 0x0000, 0x84},
        {0x577F, 0x007F, 0x80},
        {0x9B80, 0xFF80, 0x88},
        {0xDFFF, 0xFFFF, 0x88},
    };
    static const int numTests16 = sizeof(tests16) / sizeof(tests16[0]);

    int i = 0;
    writeString("EXTS (16)");

    unsigned char cc;
    unsigned int result;

    for(auto &test : tests16)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "exts %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a)
            :
        );

        if(result != test.result || cc != test.cc)
            break;

        i++;
    }

    if(i < numTests16)
    {
        char buf[60];
        auto &test = tests16[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %04X (%02X), got %04X (%02X)\n", i + 1, test.result, test.cc, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");
}
