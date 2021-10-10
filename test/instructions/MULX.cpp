#include <stdio.h>

#include "Registers.hpp"

#include "helpers.h"

void mulxu16Test()
{
    struct TestValue16
    {
        unsigned int a, b;
        unsigned long result;
    };

    static const TestValue16 tests16[]
    {
        {0x0000, 0x0000, 0x00000000},
        {0x0000, 0x0001, 0x00000000},
        {0x0001, 0x0000, 0x00000000},
        {0x0001, 0x0001, 0x00000001},
        {0x0002, 0x0003, 0x00000006},
        {0x0002, 0xFFFF, 0x0001FFFE},
        {0xFFFF, 0x0002, 0x0001FFFE},
        {0xFFFF, 0xFFFF, 0xFFFE0001},
    };
    static const int numTests16 = sizeof(tests16) / sizeof(tests16[0]);

    int i = 0;
    writeString("MULXU (16)");

    unsigned char cc;
    unsigned long result;

    for(auto &test : tests16)
    {

        long tmp = test.a;
        asm volatile(
            "ldc #0x8A, ccr\n" // does not modify flags
            "mulxu %3, %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (tmp), "r" (test.b)
            :
        );

        if(result != test.result || cc != 0x8A)
            break;

        i++;
    }

    if(i < numTests16)
    {
        char buf[80];
        auto &test = tests16[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %08lX (8F (unchanged)), got %08lX (%02X)\n", i + 1, test.result, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");
}

void mulxu8Test()
{
    struct TestValue8
    {
        unsigned char a, b;
        unsigned int result;
    };

    static const TestValue8 tests8[]
    {
        {0x00, 0x00, 0x0000},
        {0x00, 0x01, 0x0000},
        {0x01, 0x00, 0x0000},
        {0x01, 0x01, 0x0001},
        {0x02, 0x03, 0x0006},
        {0x02, 0xFF, 0x01FE},
        {0xFF, 0x02, 0x01FE},
        {0xFF, 0xFF, 0xFE01},
    };
    static const int numTests8 = sizeof(tests8) / sizeof(tests8[0]);

    int i = 0;
    writeString("MULXU (8)");

    unsigned char cc;
    unsigned int result;

    for(auto &test : tests8)
    {

        int tmp = test.a;
        asm volatile(
            "ldc #0x8F, ccr\n" // does not modify flags
            "mulxu %3l, %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (tmp), "r" (test.b)
            :
        );

        if(result != test.result || cc != 0x8F)
            break;

        i++;
    }

    if(i < numTests8)
    {
        char buf[80];
        auto &test = tests8[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %04X (8F (unchanged)), got %04X (%02X)\n", i + 1, test.result, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");
}

void mulxs16Test()
{
    struct TestValue16
    {
        unsigned int a, b;
        unsigned long result;
        unsigned char cc;
    };

    static const TestValue16 tests16[]
    {
        {0x0000, 0x0000, 0x00000000, 0x84}, // Z
        {0x0000, 0x0001, 0x00000000, 0x84}, // Z
        {0x0001, 0x0000, 0x00000000, 0x84}, // Z
        {0x0001, 0x0001, 0x00000001, 0x80},
        {0x0002, 0x0003, 0x00000006, 0x80},
        {0x0002, 0xFFFF, 0xFFFFFFFE, 0x88}, // N
        {0xFFFF, 0x0002, 0xFFFFFFFE, 0x88}, // N
        {0xFFFF, 0xFFFF, 0x00000001, 0x80},
    };
    static const int numTests16 = sizeof(tests16) / sizeof(tests16[0]);

    int i = 0;
    writeString("MULXS (16)");

    unsigned char cc;
    unsigned long result;

    for(auto &test : tests16)
    {

        long tmp = test.a;
        asm volatile(
            "ldc #0x80, ccr\n"
            "mulxs %3, %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (tmp), "r" (test.b)
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
        snprintf(buf, sizeof(buf), " failed at %i. Expected %08lX (%02X), got %08lX (%02X)\n", i + 1, test.result, test.cc, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");
}

void mulxs8Test()
{
    struct TestValue8
    {
        unsigned char a, b;
        unsigned int result;
        unsigned char cc;
    };

    static const TestValue8 tests8[]
    {
        {0x00, 0x00, 0x0000, 0x84}, // Z
        {0x00, 0x01, 0x0000, 0x84}, // Z
        {0x01, 0x00, 0x0000, 0x84}, // Z
        {0x01, 0x01, 0x0001, 0x80},
        {0x02, 0x03, 0x0006, 0x80},
        {0x02, 0xFF, 0xFFFE, 0x88}, // N
        {0xFF, 0x02, 0xFFFE, 0x88}, // N
        {0xFF, 0xFF, 0x0001, 0x80},
    };
    static const int numTests8 = sizeof(tests8) / sizeof(tests8[0]);

    int i = 0;
    writeString("MULXS (8)");

    unsigned char cc;
    unsigned int result;

    for(auto &test : tests8)
    {

        int tmp = test.a;
        asm volatile(
            "ldc #0x80, ccr\n"
            "mulxs %3l, %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (tmp), "r" (test.b)
            :
        );

        if(result != test.result || cc != test.cc)
            break;

        i++;
    }

    if(i < numTests8)
    {
        char buf[60];
        auto &test = tests8[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %04X (%02X), got %04X (%02X)\n", i + 1, test.result, test.cc, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");
}
