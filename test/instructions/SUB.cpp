#include <stdio.h>

#include "Registers.hpp"

#include "helpers.h"

void sub32Test()
{
    struct TestValue32
    {
        unsigned long a, b, result;
        unsigned char cc;
    };

    static const TestValue32 tests32[]
    {
        {0x00000001, 0x00000000, 0x00000001, 0x80}, // none
        {0x0FFFFFFF, 0x0FFFFFFE, 0x00000001, 0x80}, // none
        {0x80000000, 0x00000000, 0x80000000, 0x88}, // N
        {0x00000000, 0x00000000, 0x00000000, 0x84}, // Z
        {0x00000001, 0x00000001, 0x00000000, 0x84}, // Z
        {0x80000000, 0x10000000, 0x70000000, 0x82}, // V
        {0x00000000, 0x90000000, 0x70000000, 0x81}, // C
        {0x00000000, 0x10000000, 0xF0000000, 0x89}, // NC
        {0x00000000, 0x80000000, 0x80000000, 0x8B}, // NVC
        {0x10000000, 0x00000001, 0x0FFFFFFF, 0xA0}, // H
        {0x90000000, 0x00000001, 0x8FFFFFFF, 0xA8}, // HN
        {0x80000000, 0x00000001, 0x7FFFFFFF, 0xA2}, // HV
        {0x00000000, 0x80000001, 0x7FFFFFFF, 0xA1}, // HC
        {0x00000000, 0x00000001, 0xFFFFFFFF, 0xA9}, // HNC
        {0x10000000, 0x80000001, 0x8FFFFFFF, 0xAB}, // HNVC
    };
    static const int numTests32 = sizeof(tests32) / sizeof(tests32[0]);

    int i = 0;
    writeString("SUB IMM (32)");

    unsigned char cc;
    unsigned long result;

    // must unroll this loop to use constants
    #pragma GCC unroll 32
    for(auto &test : tests32)
    {
        asm volatile(
            "sub %3, %2\n"
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

    writeString("SUB REG (32)");
    i = 0;

    for(auto &test : tests32)
    {
        asm volatile(
            "sub %3, %2\n"
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

void sub16Test()
{
    struct TestValue16
    {
        unsigned int a, b, result;
        unsigned char cc;
    };

    static const TestValue16 tests16[]
    {
        {0x0001, 0x0000, 0x0001, 0x80}, // none
        {0x0FFF, 0x0FFE, 0x0001, 0x80}, // none
        {0x8000, 0x0000, 0x8000, 0x88}, // N
        {0x0000, 0x0000, 0x0000, 0x84}, // Z
        {0x0001, 0x0001, 0x0000, 0x84}, // Z
        {0x8000, 0x1000, 0x7000, 0x82}, // V
        {0x0000, 0x9000, 0x7000, 0x81}, // C
        {0x0000, 0x1000, 0xF000, 0x89}, // NC
        {0x0000, 0x8000, 0x8000, 0x8B}, // NVC
        {0x1000, 0x0001, 0x0FFF, 0xA0}, // H
        {0x9000, 0x0001, 0x8FFF, 0xA8}, // HN
        {0x8000, 0x0001, 0x7FFF, 0xA2}, // HV
        {0x0000, 0x8001, 0x7FFF, 0xA1}, // HC
        {0x0000, 0x0001, 0xFFFF, 0xA9}, // HNC
        {0x1000, 0x8001, 0x8FFF, 0xAB}, // HNVC
    };
    static const int numTests16 = sizeof(tests16) / sizeof(tests16[0]);

    int i = 0;
    writeString("SUB IMM (16)");

    unsigned char cc;
    unsigned int result;

    // must unroll this loop to use constants
    #pragma GCC unroll 32
    for(auto &test : tests16)
    {
        asm volatile(
            "sub %3, %2\n"
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

    writeString("SUB REG (16)");
    i = 0;

    for(auto &test : tests16)
    {
        asm volatile(
            "sub %3, %2\n"
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

void sub8Test()
{
    struct TestValue8
    {
        unsigned char a, b, result;
        unsigned char cc;
    };

    static const TestValue8 tests8[]
    {
        {0x01, 0x00, 0x01, 0x80}, // none
        {0x0F, 0x0E, 0x01, 0x80}, // none
        {0x80, 0x00, 0x80, 0x88}, // N
        {0x00, 0x00, 0x00, 0x84}, // Z
        {0x01, 0x01, 0x00, 0x84}, // Z
        {0x80, 0x10, 0x70, 0x82}, // V
        {0x00, 0x90, 0x70, 0x81}, // C
        {0x00, 0x10, 0xF0, 0x89}, // NC
        {0x00, 0x80, 0x80, 0x8B}, // NVC
        {0x10, 0x01, 0x0F, 0xA0}, // H
        {0x90, 0x01, 0x8F, 0xA8}, // HN
        {0x80, 0x01, 0x7F, 0xA2}, // HV
        {0x00, 0x81, 0x7F, 0xA1}, // HC
        {0x00, 0x01, 0xFF, 0xA9}, // HNC
        {0x10, 0x81, 0x8F, 0xAB}, // HNVC
    };
    static const int numTests8 = sizeof(tests8) / sizeof(tests8[0]);

    // there is no sub.b #imm

    int i = 0;
    unsigned char cc;
    unsigned char result;

    writeString("SUB REG (8)");

    for(auto &test : tests8)
    {
        asm volatile(
            "sub %3l, %2l\n"
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

void subxTest()
{
    struct TestValue8
    {
        unsigned char a, b, result;
        unsigned char inCC, cc;
    };

    static const TestValue8 tests8[]
    {
        {0x01, 0x00, 0x01, 0x80, 0x80}, // none
        {0x0F, 0x0E, 0x01, 0x80, 0x80}, // none
        {0x80, 0x00, 0x80, 0x80, 0x88}, // N
        {0x00, 0x00, 0x00, 0x80, 0x80}, // none (no Z)
        {0x01, 0x01, 0x00, 0x80, 0x80}, // none (no Z)
        {0x80, 0x10, 0x70, 0x80, 0x82}, // V
        {0x00, 0x90, 0x70, 0x80, 0x81}, // C
        {0x00, 0x10, 0xF0, 0x80, 0x89}, // NC
        {0x00, 0x80, 0x80, 0x80, 0x8B}, // NVC
        {0x10, 0x01, 0x0F, 0x80, 0xA0}, // H
        {0x90, 0x01, 0x8F, 0x80, 0xA8}, // HN
        {0x80, 0x01, 0x7F, 0x80, 0xA2}, // HV
        {0x00, 0x81, 0x7F, 0x80, 0xA1}, // HC
        {0x00, 0x01, 0xFF, 0x80, 0xA9}, // HNC
        {0xFE, 0xFF, 0xFF, 0x80, 0xA9}, // HNC
        {0x10, 0x81, 0x8F, 0x80, 0xAB}, // HNVC

        // with C set
        {0x02, 0x00, 0x01, 0x81, 0x80}, // none
        {0x81, 0x00, 0x80, 0x81, 0x88}, // N
        {0x01, 0x00, 0x00, 0x81, 0x80}, // none (no Z)
        {0x81, 0x10, 0x70, 0x81, 0x82}, // V
        {0x01, 0x90, 0x70, 0x81, 0x81}, // C
        {0x01, 0x10, 0xF0, 0x81, 0x89}, // NC
        {0x01, 0x80, 0x80, 0x81, 0x8B}, // NVC
        {0x10, 0x00, 0x0F, 0x81, 0xA0}, // H
        {0x90, 0x00, 0x8F, 0x81, 0xA8}, // HN
        {0x80, 0x00, 0x7F, 0x81, 0xA2}, // HV
        {0x00, 0x80, 0x7F, 0x81, 0xA1}, // HC
        {0x00, 0x00, 0xFF, 0x81, 0xA9}, // HNC
        {0xFE, 0xFF, 0xFE, 0x81, 0xA9}, // HNC
        {0x10, 0x80, 0x8F, 0x81, 0xAB}, // HNVC

        {0x02, 0x00, 0x01, 0x8F, 0x80}, // Z flag is always cleared
    };
    static const int numTests8 = sizeof(tests8) / sizeof(tests8[0]);

    int i = 0;
    writeString("SUBX IMM");

    unsigned char cc;
    unsigned char result;

    // must unroll this loop to use constants
    #pragma GCC unroll 32
    for(auto &test : tests8)
    {
        asm volatile(
            "ldc %4, ccr\n"
            "subx %3, %2l\n"
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

    writeString("SUBX REG");
    i = 0;

    for(auto &test : tests8)
    {
        asm volatile(
            "ldc %4l, ccr\n"
            "subx %3l, %2l\n"
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

void subsTest()
{
    struct TestValue
    {
        unsigned long a, b, result;
    };

    static const TestValue tests[]
    {
        // these don't set flags
        {0x00000000, 1, 0xFFFFFFFF},
        {0x00000000, 2, 0xFFFFFFFE},
        {0x00000000, 4, 0xFFFFFFFC},
        {0x0FFFFFFF, 1, 0x0FFFFFFE},
        {0x0FFFFFFF, 2, 0x0FFFFFFD},
        {0x0FFFFFFF, 4, 0x0FFFFFFB},
        {0x80000000, 1, 0x7FFFFFFF},
        {0x80000000, 2, 0x7FFFFFFE},
        {0x80000000, 4, 0x7FFFFFFC},
        {0x90000000, 1, 0x8FFFFFFF},
        {0x90000000, 2, 0x8FFFFFFE},
        {0x90000000, 4, 0x8FFFFFFC},
        {0x7FFFFFFF, 1, 0x7FFFFFFE},
        {0x7FFFFFFF, 2, 0x7FFFFFFD},
        {0x7FFFFFFF, 4, 0x7FFFFFFB},
        {0xFFFFFFFF, 1, 0xFFFFFFFE},
        {0xFFFFFFFF, 2, 0xFFFFFFFD},
        {0xFFFFFFFF, 4, 0xFFFFFFFB},
    };
    static const int numTests = sizeof(tests) / sizeof(tests[0]);

    int i = 0;
    writeString("SUBS");

    unsigned char cc;
    unsigned long result;

    // must unroll this loop to use constants
    #pragma GCC unroll 32
    for(auto &test : tests)
    {
        asm volatile(
            "ldc #0x8F, ccr\n" // doesn't modify flags
            "subs %3, %2\n"
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
