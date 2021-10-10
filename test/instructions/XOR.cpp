#include <stdio.h>

#include "Registers.hpp"

#include "helpers.h"

void xor32Test()
{
    struct TestValue32
    {
        unsigned long a, b, result;
        unsigned char cc;
    };

    static const TestValue32 tests32[]
    {
        {0x00000000, 0x00000000, 0x00000000, 0x84},
        {0x00000001, 0x00000001, 0x00000000, 0x84},
        {0x80000000, 0x00000000, 0x80000000, 0x88},
        {0x80000000, 0x80000000, 0x00000000, 0x84},
        {0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x84},
        {0xAAAAAAAA, 0x55555555, 0xFFFFFFFF, 0x88},
        {0x01234567, 0x89ABCDEF, 0x88888888, 0x88},
        {0xFFFFFFFF, 0xAAAAAAAA, 0x55555555, 0x80},
    };
    static const int numTests32 = sizeof(tests32) / sizeof(tests32[0]);

    int i = 0;
    writeString("XOR IMM (32)");

    unsigned char cc;
    unsigned long result;

    // must unroll this loop to use constants
    #pragma GCC unroll 32
    for(auto &test : tests32)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "xor %3, %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a), "i" (test.b)
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

    writeString("XOR REG (32)");
    i = 0;

    for(auto &test : tests32)
    {
        asm volatile(
            "ldc #0x8F, ccr\n"
            "xor %3, %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a), "r" (test.b)
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

void xor16Test()
{
    struct TestValue16
    {
        unsigned int a, b, result;
        unsigned char cc;
    };

    static const TestValue16 tests16[]
    {
        {0x0000, 0x0000, 0x0000, 0x84},
        {0x0001, 0x0001, 0x0000, 0x84},
        {0x8000, 0x0000, 0x8000, 0x88},
        {0x8000, 0x8000, 0x0000, 0x84},
        {0xFFFF, 0xFFFF, 0x0000, 0x84},
        {0xAAAA, 0x5555, 0xFFFF, 0x88},
        {0x1357, 0x9BDF, 0x8888, 0x88},
        {0xFFFF, 0xAAAA, 0x5555, 0x80},
    };
    static const int numTests16 = sizeof(tests16) / sizeof(tests16[0]);

    int i = 0;
    writeString("XOR IMM (16)");

    unsigned char cc;
    unsigned int result;

    // must unroll this loop to use constants
    #pragma GCC unroll 32
    for(auto &test : tests16)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "xor %3, %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a), "i" (test.b)
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

    writeString("XOR REG (16)");
    i = 0;

    for(auto &test : tests16)
    {
        asm volatile(
            "ldc #0x8F, ccr\n"
            "xor %3, %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a), "r" (test.b)
            :
        );

        if(result != test.result || cc != (test.cc | 1))
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

void xor8Test()
{
    struct TestValue8
    {
        unsigned char a, b, result;
        unsigned char cc;
    };

    static const TestValue8 tests8[]
    {
        {0x00, 0x00, 0x00, 0x84},
        {0x01, 0x01, 0x00, 0x84},
        {0x80, 0x00, 0x80, 0x88},
        {0x80, 0x80, 0x00, 0x84},
        {0xFF, 0xFF, 0x00, 0x84},
        {0xAA, 0x55, 0xFF, 0x88},
        {0x37, 0xBF, 0x88, 0x88},
        {0xFF, 0xAA, 0x55, 0x80},
    };
    static const int numTests8 = sizeof(tests8) / sizeof(tests8[0]);

    int i = 0;
    writeString("XOR IMM (8)");

    unsigned char cc;
    unsigned char result;

    // must unroll this loop to use constants
    #pragma GCC unroll 32
    for(auto &test : tests8)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "xor %3, %2l\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a), "i" (test.b)
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
        snprintf(buf, sizeof(buf), " failed at %i. Expected %02X (%02X), got %02X (%02X)\n", i + 1, test.result, test.cc, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");

    writeString("XOR REG (8)");
    i = 0;

    for(auto &test : tests8)
    {
        asm volatile(
            "ldc #0x8F, ccr\n"
            "xor %3l, %2l\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a), "r" (test.b)
            :
        );

        if(result != test.result || cc != (test.cc | 1))
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
