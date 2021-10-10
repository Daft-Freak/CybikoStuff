#include <stdio.h>

#include "Registers.hpp"

#include "helpers.h"

// basically sub with no result
void cmp32Test()
{
    struct TestValue32
    {
        unsigned long a, b;
        unsigned char cc;
    };

    static const TestValue32 tests32[]
    {
        {0x00000001, 0x00000000, 0x80}, // none
        {0x0FFFFFFF, 0x0FFFFFFE, 0x80}, // none
        {0x80000000, 0x00000000, 0x88}, // N
        {0x00000000, 0x00000000, 0x84}, // Z
        {0x00000001, 0x00000001, 0x84}, // Z
        {0x80000000, 0x10000000, 0x82}, // V
        {0x00000000, 0x90000000, 0x81}, // C
        {0x00000000, 0x10000000, 0x89}, // NC
        {0x00000000, 0x80000000, 0x8B}, // NVC
        {0x10000000, 0x00000001, 0xA0}, // H
        {0x90000000, 0x00000001, 0xA8}, // HN
        {0x80000000, 0x00000001, 0xA2}, // HV
        {0x00000000, 0x80000001, 0xA1}, // HC
        {0x00000000, 0x00000001, 0xA9}, // HNC
        {0x10000000, 0x80000001, 0xAB}, // HNVC
    };
    static const int numTests32 = sizeof(tests32) / sizeof(tests32[0]);

    int i = 0;
    writeString("CMP IMM (32)");

    unsigned char cc;
    unsigned long result;

    // must unroll this loop to use constants
    #pragma GCC unroll 32
    for(auto &test : tests32)
    {
        asm volatile(
            "cmp %3, %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a), "i" (test.b)
            :
        );

        if(cc != test.cc)
            break;

        i++;
    }

    if(i < numTests32)
    {
        char buf[60];
        auto &test = tests32[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %02X, got %02X\n", i + 1, test.cc, cc);
        writeString(buf);
    }
    else
        writeString(".\n");

    writeString("CMP REG (32)");
    i = 0;

    for(auto &test : tests32)
    {
        asm volatile(
            "cmp %3, %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a), "r" (test.b)
            :
        );

        if(cc != test.cc)
            break;

        i++;
    }

    if(i < numTests32)
    {
        char buf[60];
        auto &test = tests32[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %02X, got%02X\n", i + 1, test.cc, cc);
        writeString(buf);
    }
    else
        writeString(".\n");
}

void cmp16Test()
{
    struct TestValue16
    {
        unsigned int a, b;
        unsigned char cc;
    };

    static const TestValue16 tests16[]
    {
        {0x0001, 0x0000, 0x80}, // none
        {0x0FFF, 0x0FFE, 0x80}, // none
        {0x8000, 0x0000, 0x88}, // N
        {0x0000, 0x0000, 0x84}, // Z
        {0x0001, 0x0001, 0x84}, // Z
        {0x8000, 0x1000, 0x82}, // V
        {0x0000, 0x9000, 0x81}, // C
        {0x0000, 0x1000, 0x89}, // NC
        {0x0000, 0x8000, 0x8B}, // NVC
        {0x1000, 0x0001, 0xA0}, // H
        {0x9000, 0x0001, 0xA8}, // HN
        {0x8000, 0x0001, 0xA2}, // HV
        {0x0000, 0x8001, 0xA1}, // HC
        {0x0000, 0x0001, 0xA9}, // HNC
        {0x1000, 0x8001, 0xAB}, // HNVC
    };
    static const int numTests16 = sizeof(tests16) / sizeof(tests16[0]);

    int i = 0;
    writeString("CMP IMM (16)");

    unsigned char cc;
    unsigned int result;

    // must unroll this loop to use constants
    #pragma GCC unroll 32
    for(auto &test : tests16)
    {
        asm volatile(
            "cmp %3, %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a), "i" (test.b)
            :
        );

        if(cc != test.cc)
            break;

        i++;
    }

    if(i < numTests16)
    {
        char buf[60];
        auto &test = tests16[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %02X, got %02Xn", i + 1, test.cc, cc);
        writeString(buf);
    }
    else
        writeString(".\n");

    writeString("CMP REG (16)");
    i = 0;

    for(auto &test : tests16)
    {
        asm volatile(
            "cmp %3, %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a), "r" (test.b)
            :
        );

        if(cc != test.cc)
            break;

        i++;
    }

    if(i < numTests16)
    {
        char buf[60];
        auto &test = tests16[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %02X, got %02X\n", i + 1, test.cc, cc);
        writeString(buf);
    }
    else
        writeString(".\n");
}


void cmp8Test()
{
    struct TestValue8
    {
        unsigned char a, b;
        unsigned char cc;
    };

    static const TestValue8 tests8[]
    {
        {0x01, 0x00, 0x80}, // none
        {0x0F, 0x0E, 0x80}, // none
        {0x80, 0x00, 0x88}, // N
        {0x00, 0x00, 0x84}, // Z
        {0x01, 0x01, 0x84}, // Z
        {0x80, 0x10, 0x82}, // V
        {0x00, 0x90, 0x81}, // C
        {0x00, 0x10, 0x89}, // NC
        {0x00, 0x80, 0x8B}, // NVC
        {0x10, 0x01, 0xA0}, // H
        {0x90, 0x01, 0xA8}, // HN
        {0x80, 0x01, 0xA2}, // HV
        {0x00, 0x81, 0xA1}, // HC
        {0x00, 0x01, 0xA9}, // HNC
        {0x10, 0x81, 0xAB}, // HNVC
    };
    static const int numTests8 = sizeof(tests8) / sizeof(tests8[0]);

    int i = 0;
    writeString("CMP IMM (8)");

    unsigned char cc;
    unsigned char result;

    // must unroll this loop to use constants
    #pragma GCC unroll 32
    for(auto &test : tests8)
    {
        asm volatile(
            "cmp %3, %2l\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a), "i" (test.b)
            :
        );

        if(cc != test.cc)
            break;

        i++;
    }

    if(i < numTests8)
    {
        char buf[60];
        auto &test = tests8[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %02X, got %02X\n", i + 1, test.cc, cc);
        writeString(buf);
    }
    else
        writeString(".\n");

    writeString("CMP REG (8)");
    i = 0;

    for(auto &test : tests8)
    {
        asm volatile(
            "cmp %3l, %2l\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a), "r" (test.b)
            :
        );

        if(cc != test.cc)
            break;

        i++;
    }

    if(i < numTests8)
    {
        char buf[60];
        auto &test = tests8[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %02X, got %02X\n", i + 1, test.cc, cc);
        writeString(buf);
    }
    else
        writeString(".\n");
}
