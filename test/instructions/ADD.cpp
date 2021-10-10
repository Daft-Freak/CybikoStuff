#include <stdio.h>

#include "Registers.hpp"

#include "helpers.h"

void add32Test()
{
    struct TestValue32
    {
        unsigned long a, b, result;
        unsigned char cc;
    };

    static const TestValue32 tests32[]
    {
        {0x00000001, 0x00000000, 0x00000001, 0x80}, // none
        {0x00000001, 0x0FFFFFFE, 0x0FFFFFFF, 0x80}, // none
        {0x80000000, 0x00000000, 0x80000000, 0x88}, // N
        {0x00000000, 0x00000000, 0x00000000, 0x84}, // Z
        {0xF0000001, 0x10000000, 0x00000001, 0x81}, // C
        {0x70000000, 0x10000000, 0x80000000, 0x8A}, // NV
        {0xF0000000, 0x90000000, 0x80000000, 0x89}, // NC
        {0xF0000000, 0x10000000, 0x00000000, 0x85}, // ZC
        {0x80000000, 0x80000001, 0x00000001, 0x83}, // VC
        {0x80000000, 0x80000000, 0x00000000, 0x87}, // ZVC
        {0x0FFFFFFF, 0x00000001, 0x10000000, 0xA0}, // H
        {0x8FFFFFFF, 0x00000001, 0x90000000, 0xA8}, // HN
        {0xFFFFFFFF, 0x00000002, 0x00000001, 0xA1}, // HC
        {0x7FFFFFFF, 0x00000001, 0x80000000, 0xAA}, // HNV
        {0xFFFFFFFF, 0x80000001, 0x80000000, 0xA9}, // HNC
        {0xFFFFFFFF, 0x00000001, 0x00000000, 0xA5}, // HZC
        {0x8FFFFFFF, 0x80000001, 0x10000000, 0xA3}, // HVC
    };
    static const int numTests32 = sizeof(tests32) / sizeof(tests32[0]);

    int i = 0;
    writeString("ADD IMM (32)");

    unsigned char cc;
    unsigned long result;

    // must unroll this loop to use constants
    #pragma GCC unroll 32
    for(auto &test : tests32)
    {
        asm volatile(
            "add %3, %2\n"
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

    writeString("ADD REG (32)");
    i = 0;

    for(auto &test : tests32)
    {
        asm volatile(
            "add %3, %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a), "r" (test.b)
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

void add16Test()
{
    struct TestValue16
    {
        unsigned int a, b, result;
        unsigned char cc;
    };

    static const TestValue16 tests16[]
    {
        {0x0001, 0x0000, 0x0001, 0x80}, // none
        {0x0001, 0x0FFE, 0x0FFF, 0x80}, // none
        {0x8000, 0x0000, 0x8000, 0x88}, // N
        {0x0000, 0x0000, 0x0000, 0x84}, // Z
        {0xF001, 0x1000, 0x0001, 0x81}, // C
        {0x7000, 0x1000, 0x8000, 0x8A}, // NV
        {0xF000, 0x9000, 0x8000, 0x89}, // NC
        {0xF000, 0x1000, 0x0000, 0x85}, // ZC
        {0x8000, 0x8001, 0x0001, 0x83}, // VC
        {0x8000, 0x8000, 0x0000, 0x87}, // ZVC
        {0x0FFF, 0x0001, 0x1000, 0xA0}, // H
        {0x8FFF, 0x0001, 0x9000, 0xA8}, // HN
        {0xFFFF, 0x0002, 0x0001, 0xA1}, // HC
        {0x7FFF, 0x0001, 0x8000, 0xAA}, // HNV
        {0xFFFF, 0x8001, 0x8000, 0xA9}, // HNC
        {0xFFFF, 0x0001, 0x0000, 0xA5}, // HZC
        {0x8FFF, 0x8001, 0x1000, 0xA3}, // HVC
    };
    static const int numTests16 = sizeof(tests16) / sizeof(tests16[0]);

    int i = 0;
    writeString("ADD IMM (16)");

    unsigned char cc;
    unsigned int result;

    // must unroll this loop to use constants
    #pragma GCC unroll 32
    for(auto &test : tests16)
    {
        asm volatile(
            "add %3, %2\n"
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

    writeString("ADD REG (16)");
    i = 0;

    for(auto &test : tests16)
    {
        asm volatile(
            "add %3, %2\n"
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
        snprintf(buf, sizeof(buf), " failed at %i. Expected %04X (%02X), got %04X (%02X)\n", i + 1, test.result, test.cc, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");
}

void add8Test()
{
    struct TestValue8
    {
        unsigned char a, b, result;
        unsigned char cc;
    };

    static const TestValue8 tests8[]
    {
        {0x01, 0x00, 0x01, 0x80}, // none
        {0x01, 0x0E, 0x0F, 0x80}, // none
        {0x80, 0x00, 0x80, 0x88}, // N
        {0x00, 0x00, 0x00, 0x84}, // Z
        {0xF1, 0x10, 0x01, 0x81}, // C
        {0x70, 0x10, 0x80, 0x8A}, // NV
        {0xF0, 0x90, 0x80, 0x89}, // NC
        {0xF0, 0x10, 0x00, 0x85}, // ZC
        {0x80, 0x81, 0x01, 0x83}, // VC
        {0x80, 0x80, 0x00, 0x87}, // ZVC
        {0x0F, 0x01, 0x10, 0xA0}, // H
        {0x8F, 0x01, 0x90, 0xA8}, // HN
        {0xFF, 0x02, 0x01, 0xA1}, // HC
        {0x7F, 0x01, 0x80, 0xAA}, // HNV
        {0xFF, 0x81, 0x80, 0xA9}, // HNC
        {0xFF, 0x01, 0x00, 0xA5}, // HZC
        {0x8F, 0x81, 0x10, 0xA3}, // HVC
    };
    static const int numTests8 = sizeof(tests8) / sizeof(tests8[0]);

    int i = 0;
    writeString("ADD IMM (8)");

    unsigned char cc;
    unsigned char result;

    // must unroll this loop to use constants
    #pragma GCC unroll 32
    for(auto &test : tests8)
    {
        asm volatile(
            "add %3, %2l\n"
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

    writeString("ADD REG (8)");
    i = 0;

    for(auto &test : tests8)
    {
        asm volatile(
            "add %3l, %2l\n"
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
        snprintf(buf, sizeof(buf), " failed at %i. Expected %02X (%02X), got %02X (%02X)\n", i + 1, test.result, test.cc, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");
}

void addxTest()
{
    struct TestValue8
    {
        unsigned char a, b, result;
        unsigned char inCC, cc;
    };

    static const TestValue8 tests8[]
    {
        {0x01, 0x00, 0x01, 0x80, 0x80}, // none
        {0x01, 0x0E, 0x0F, 0x80, 0x80}, // none
        {0x80, 0x00, 0x80, 0x80, 0x88}, // N
        {0x00, 0x00, 0x00, 0x80, 0x80}, // none (no Z)
        {0xF1, 0x10, 0x01, 0x80, 0x81}, // C
        {0x70, 0x10, 0x80, 0x80, 0x8A}, // NV
        {0xF0, 0x90, 0x80, 0x80, 0x89}, // NC
        {0xF0, 0x10, 0x00, 0x80, 0x81}, // C (no Z)
        {0x80, 0x81, 0x01, 0x80, 0x83}, // VC
        {0x80, 0x80, 0x00, 0x80, 0x83}, // VC (no Z)
        {0x0F, 0x01, 0x10, 0x80, 0xA0}, // H
        {0x8F, 0x01, 0x90, 0x80, 0xA8}, // HN
        {0xFF, 0x02, 0x01, 0x80, 0xA1}, // HC
        {0x7F, 0x01, 0x80, 0x80, 0xAA}, // HNV
        {0xFF, 0x81, 0x80, 0x80, 0xA9}, // HNC
        {0xFF, 0x01, 0x00, 0x80, 0xA1}, // HC (no Z)
        {0x8F, 0x81, 0x10, 0x80, 0xA3}, // HVC

        // with C set
        {0x00, 0x00, 0x01, 0x81, 0x80}, // none
        {0x80, 0x00, 0x81, 0x81, 0x88}, // N
        {0xF0, 0x10, 0x01, 0x81, 0x81}, // C
        {0x70, 0x10, 0x81, 0x81, 0x8A}, // NV
        {0xF0, 0x90, 0x81, 0x81, 0x89}, // NC
        {0x80, 0x80, 0x01, 0x81, 0x83}, // VC
        {0x0F, 0x00, 0x10, 0x81, 0xA0}, // H
        {0x8F, 0x00, 0x90, 0x81, 0xA8}, // HN
        {0xFF, 0x01, 0x01, 0x81, 0xA1}, // HC
        {0x7F, 0x00, 0x80, 0x81, 0xAA}, // HNV
        {0xFF, 0x80, 0x80, 0x81, 0xA9}, // HNC
        {0xFF, 0x00, 0x00, 0x81, 0xA1}, // HC (no Z)
        {0x00, 0xFF, 0x00, 0x81, 0xA1}, // HC (no Z)
        {0x8F, 0x80, 0x10, 0x81, 0xA3}, // HVC

        {0x01, 0x00, 0x02, 0x8F, 0x80}, // Z flag is always cleared
    };
    static const int numTests8 = sizeof(tests8) / sizeof(tests8[0]);

    int i = 0;
    writeString("ADDX IMM");

    unsigned char cc;
    unsigned char result;

    // must unroll this loop to use constants
    #pragma GCC unroll 32
    for(auto &test : tests8)
    {
        asm volatile(
            "ldc %4, ccr\n"
            "addx %3, %2l\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a), "i" (test.b), "i" (test.inCC)
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

    writeString("ADDX REG");
    i = 0;

    for(auto &test : tests8)
    {
        asm volatile(
            "ldc %4l, ccr\n"
            "addx %3l, %2l\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a), "r" (test.b), "r" (test.inCC)
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
}

void addsTest()
{
    struct TestValue
    {
        unsigned long a, b, result;
    };

    static const TestValue tests[]
    {
        // these don't set flags
        {0x00000000, 1, 0x00000001},
        {0x00000000, 2, 0x00000002},
        {0x00000000, 4, 0x00000004},
        {0x0FFFFFFE, 1, 0x0FFFFFFF},
        {0x0FFFFFFD, 2, 0x0FFFFFFF},
        {0x0FFFFFFB, 4, 0x0FFFFFFF},
        {0x80000000, 1, 0x80000001},
        {0x80000000, 2, 0x80000002},
        {0x80000000, 4, 0x80000004},
        {0x0FFFFFFF, 1, 0x10000000},
        {0x0FFFFFFF, 2, 0x10000001},
        {0x0FFFFFFF, 4, 0x10000003},
        {0x8FFFFFFF, 1, 0x90000000},
        {0x8FFFFFFF, 2, 0x90000001},
        {0x8FFFFFFF, 4, 0x90000003},
        {0x7FFFFFFF, 1, 0x80000000},
        {0x7FFFFFFF, 2, 0x80000001},
        {0x7FFFFFFF, 4, 0x80000003},
        {0xFFFFFFFF, 1, 0x00000000},
        {0xFFFFFFFF, 2, 0x00000001},
        {0xFFFFFFFF, 4, 0x00000003},
    };
    static const int numTests = sizeof(tests) / sizeof(tests[0]);

    int i = 0;
    writeString("ADDS");

    unsigned char cc;
    unsigned long result;

    // must unroll this loop to use constants
    #pragma GCC unroll 32
    for(auto &test : tests)
    {
        asm volatile(
            "ldc #0x8F, ccr\n" // doesn't modify flags
            "adds %3, %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a), "L" (test.b)
            :
        );

        if(result != test.result || cc != 0x8F)
            break;

        i++;
    }

    if(i < numTests)
    {
        char buf[80];
        auto &test = tests[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %08lX (8F (unchanged)), got %08lX (%02X)\n", i + 1, test.result, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");
}
