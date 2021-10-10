#include <stdio.h>

#include "Registers.hpp"

#include "helpers.h"


void not32Test()
{
    struct TestValue32
    {
        unsigned long a, result;
        unsigned char cc;
    };

    static const TestValue32 tests32[]
    {
        {0x00000000, 0xFFFFFFFF, 0x88},
        {0x00000001, 0xFFFFFFFE, 0x88},
        {0x80000000, 0x7FFFFFFF, 0x80},
        {0x7FFFFFFF, 0x80000000, 0x88},
        {0xFFFFFFFF, 0x00000000, 0x84},
        {0xAAAAAAAA, 0x55555555, 0x80},
        {0x01234567, 0xFEDCBA98, 0x88},
        {0x89ABCDEF, 0x76543210, 0x80},
    };
    static const int numTests32 = sizeof(tests32) / sizeof(tests32[0]);

    int i = 0;

    unsigned char cc;
    unsigned long result;

    writeString("NOT (32)");

    for(auto &test : tests32)
    {
        asm volatile(
            "ldc #0x8F, ccr\n"
            "not %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a)
            :
        );

        if(result != test.result || cc != (test.cc | 1)) // carry not changed
            break;

        i++;
    }

    if(i < numTests32)
    {
        char buf[60];
        auto &test = tests32[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %08lX (%02X), got %08lX (%02X)\n", i + 1, test.result, test.cc | 1, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");
}

void not16Test()
{
    struct TestValue16
    {
        unsigned int a, result;
        unsigned char cc;
    };

    static const TestValue16 tests16[]
    {
        {0x0000, 0xFFFF, 0x88},
        {0x0001, 0xFFFE, 0x88},
        {0x8000, 0x7FFF, 0x80},
        {0x7FFF, 0x8000, 0x88},
        {0xFFFF, 0x0000, 0x84},
        {0xAAAA, 0x5555, 0x80},
        {0x1357, 0xECA8, 0x88},
        {0x9BDF, 0x6420, 0x80},
    };
    static const int numTests16 = sizeof(tests16) / sizeof(tests16[0]);

    int i = 0;

    unsigned char cc;
    unsigned int result;

    writeString("NOT (16)");

    for(auto &test : tests16)
    {
        asm volatile(
            "ldc #0x8F, ccr\n"
            "not %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a)
            :
        );

        if(result != test.result || cc != (test.cc | 1)) // carry not changed
            break;

        i++;
    }

    if(i < numTests16)
    {
        char buf[60];
        auto &test = tests16[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %04X (%02X), got %04X (%02X)\n", i + 1, test.result, test.cc | 1, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");
}

void not8Test()
{
    struct TestValue8
    {
        unsigned char a, result;
        unsigned char cc;
    };

    static const TestValue8 tests8[]
    {
        {0x00, 0xFF, 0x88},
        {0x01, 0xFE, 0x88},
        {0x80, 0x7F, 0x80},
        {0x7F, 0x80, 0x88},
        {0xFF, 0x00, 0x84},
        {0xAA, 0x55, 0x80},
        {0x37, 0xC8, 0x88},
        {0xBF, 0x40, 0x80},
    };
    static const int numTests8 = sizeof(tests8) / sizeof(tests8[0]);

    int i = 0;

    unsigned char cc;
    unsigned char result;

    writeString("NOT (8)");

    for(auto &test : tests8)
    {
        asm volatile(
            "ldc #0x8F, ccr\n"
            "not %2l\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a)
            :
        );

        if(result != test.result || cc != (test.cc | 1)) // carry not changed
            break;

        i++;
    }

    if(i < numTests8)
    {
        char buf[60];
        auto &test = tests8[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %02X (%02X), got %02X (%02X)\n", i + 1, test.result, test.cc | 1, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");
}

