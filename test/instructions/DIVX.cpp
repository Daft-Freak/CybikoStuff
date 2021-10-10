#include <stdio.h>

#include "Registers.hpp"

#include "helpers.h"
void divxu16Test()
{
    struct TestValue16
    {
        unsigned long a;
        unsigned int b;
        unsigned long result;
        unsigned char cc;
    };

    static const TestValue16 tests16[]
    {
        {0x00000001, 0x0001, 0x00000001, 0x80}, // none
        {0x00000001, 0x0000, 0x00010000, 0x84}, // Z (divide by 0)
        {0x12345678, 0x0000, 0x56781234, 0x84}, // Z (divide by 0)
        {0xFFFFFFFF, 0x0000, 0xFFFFFFFF, 0x84}, // Z (divide by 0)
        {0x00000004, 0x0002, 0x00000002, 0x80},
        {0x00000004, 0x0003, 0x00010001, 0x80},
        {0x0000000E, 0x0005, 0x00040002, 0x80},
        {0x00000002, 0xFFFF, 0x00020000, 0x88}, // N
        {0xFFFFFFFF, 0x0002, 0x0001FFFF, 0x80},
        {0x80000000, 0xFFFF, 0x80008000, 0x88}, // N
        {0xFFFFFFFF, 0x8000, 0x7FFFFFFF, 0x88}, // N
    };
    static const int numTests16 = sizeof(tests16) / sizeof(tests16[0]);

    int i = 0;
    writeString("DIVXU (16)");

    unsigned char cc;
    unsigned long result;

    for(auto &test : tests16)
    {

        asm volatile(
            "ldc #0x80, ccr\n"
            "divxu %3, %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a), "r" (test.b)
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

void divxu8Test()
{
    struct TestValue8
    {
        unsigned int a;
        unsigned char b;
        unsigned int result;
        unsigned char cc;
    };

    static const TestValue8 tests8[]
    {
        {0x0001, 0x01, 0x0001, 0x80}, // none
        {0x0001, 0x00, 0x0100, 0x84}, // Z (divide by 0)
        {0x1234, 0x00, 0x3412, 0x84}, // Z (divide by 0)
        {0xFFFF, 0x00, 0xFFFF, 0x84}, // Z (divide by 0)
        {0x0004, 0x02, 0x0002, 0x80},
        {0x0004, 0x03, 0x0101, 0x80},
        {0x000E, 0x05, 0x0402, 0x80},
        {0x0002, 0xFF, 0x0200, 0x88}, // N
        {0xFFFF, 0x02, 0x01FF, 0x80},
        {0x8000, 0xFF, 0x8080, 0x88}, // N
        {0xFFFF, 0x80, 0x7FFF, 0x88}, // N
    };
    static const int numTests8 = sizeof(tests8) / sizeof(tests8[0]);

    int i = 0;
    writeString("DIVXU (8)");

    unsigned char cc;
    unsigned int result;

    for(auto &test : tests8)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "divxu %3l, %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a), "r" (test.b)
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


void divxs16Test()
{
    struct TestValue16
    {
        unsigned long a;
        unsigned int b;
        unsigned long result;
        unsigned char cc;
    };

    // N is set if divisor/dividend signs differ, even if result is not negative (rounded to 0, divide by 0)
    static const TestValue16 tests16[]
    {
        {0x00000001, 0x0001, 0x00000001, 0x80}, // none
        {0x00000001, 0x0000, 0x00010000, 0x84}, // Z (divide by 0)
        {0x12345678, 0x0000, 0x56781234, 0x84}, // Z (divide by 0) swaps words
        {0x87654321, 0x0000, 0x43218766, 0x8C}, // Z (divide by 0) negative gets +1
        {0xFFFFFFFF, 0x0000, 0xFFFF0000, 0x8C}, // NZ (divide by 0)
        {0x00000002, 0xFFFF, 0x0000FFFE, 0x88}, // N
        {0x00000004, 0x0002, 0x00000002, 0x80},
        {0x00000004, 0x0003, 0x00010001, 0x80},
        {0x0000000E, 0x0005, 0x00040002, 0x80},
        {0x00000001, 0x0002, 0x00010000, 0x80},
        {0xFFFFFFFF, 0x0002, 0xFFFF0000, 0x88}, // N even though the result is 0
        {0xFFFFFFFF, 0xFFFE, 0xFFFF0000, 0x80},
        {0x80000001, 0xFFFF, 0x0000FFFF, 0x80}, // result is 0x7FFFFFFF truncated
        {0x80000000, 0xFFFF, 0xFFFFFFFF, 0x80}, // result overflows
        {0xFFFFFFFF, 0x8000, 0xFFFF0000, 0x80},
    };
    static const int numTests16 = sizeof(tests16) / sizeof(tests16[0]);

    int i = 0;
    writeString("DIVXS (16)");

    unsigned char cc;
    unsigned long result;

    for(auto &test : tests16)
    {

        long tmp = test.a;
        asm volatile(
            "ldc #0x80, ccr\n"
            "divxs %3, %2\n"
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


void divxs8Test()
{
    struct TestValue8
    {
        unsigned int a;
        unsigned char b;
        unsigned int result;
        unsigned char cc;
    };

    static const TestValue8 tests8[]
    {
        {0x0001, 0x01, 0x0001, 0x80}, // none
        {0x0001, 0x00, 0x0100, 0x84}, // Z (divide by 0)
        {0x1234, 0x00, 0x3412, 0x84}, // Z (divide by 0) swaps words
        {0x8765, 0x00, 0x6588, 0x8C}, // Z (divide by 0) negative gets +1
        {0xFFFF, 0x00, 0xFF00, 0x8C}, // NZ (divide by 0)
        {0x0002, 0xFF, 0x00FE, 0x88}, // N
        {0x0004, 0x02, 0x0002, 0x80},
        {0x0004, 0x03, 0x0101, 0x80},
        {0x000E, 0x05, 0x0402, 0x80},
        {0x0001, 0x02, 0x0100, 0x80},
        {0xFFFF, 0x02, 0xFF00, 0x88}, // N even though the result is 0
        {0xFFFF, 0xFE, 0xFF00, 0x80},
        {0x8001, 0xFF, 0x00FF, 0x80}, // result is 0x7FFFFFFF truncated
        {0x8000, 0xFF, 0xFFFF, 0x80}, // result overflows
        {0xFFFF, 0x80, 0xFF00, 0x80},
    };
    static const int numTests8 = sizeof(tests8) / sizeof(tests8[0]);

    int i = 0;
    writeString("DIVXS (8)");

    unsigned char cc;
    unsigned int result;

    for(auto &test : tests8)
    {

        int tmp = test.a;
        asm volatile(
            "ldc #0x80, ccr\n"
            "divxs %3l, %2\n"
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
