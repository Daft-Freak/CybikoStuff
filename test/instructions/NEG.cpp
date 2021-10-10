#include <stdio.h>

#include "Registers.hpp"

#include "helpers.h"

void neg32Test()
{
    struct TestValue32
    {
        unsigned long a, result;
        unsigned char cc;
    };

    static const TestValue32 tests32[]
    {
        {0x00000000, 0x00000000, 0x84}, // Z
        {0x90000000, 0x70000000, 0x81}, // C
        {0x10000000, 0xF0000000, 0x89}, // NC
        {0x80000001, 0x7FFFFFFF, 0xA1}, // HC
        {0x00000001, 0xFFFFFFFF, 0xA9}, // HNC
        {0x80000000, 0x80000000, 0x8B}, // NVC
    };
    static const int numTests32 = sizeof(tests32) / sizeof(tests32[0]);

    int i = 0;
    unsigned char cc;
    unsigned long result;

    writeString("NEG (32)");

    for(auto &test : tests32)
    {
        asm volatile(
            "neg %2\n"
            "stc ccr, r0l\n"
            "mov r0, %1\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a)
            : "r0"
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

void neg16Test()
{
    struct TestValue16
    {
        unsigned int a, result;
        unsigned char cc;
    };

    static const TestValue16 tests16[]
    {
        {0x0000, 0x0000, 0x84}, // Z
        {0x9000, 0x7000, 0x81}, // C
        {0x1000, 0xF000, 0x89}, // NC
        {0x8001, 0x7FFF, 0xA1}, // HC
        {0x0001, 0xFFFF, 0xA9}, // HNC
        {0x8000, 0x8000, 0x8B}, // NVC
    };
    static const int numTests16 = sizeof(tests16) / sizeof(tests16[0]);

    int i = 0;
    unsigned char cc;
    unsigned int result;

    writeString("NEG (16)");

    for(auto &test : tests16)
    {
        asm volatile(
            "neg %2\n"
            "stc ccr, r0l\n"
            "mov r0, %1\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a)
            : "r0"
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

void neg8Test()
{
    struct TestValue8
    {
        unsigned char a, result;
        unsigned char cc;
    };

    static const TestValue8 tests8[]
    {
        {0x00, 0x00, 0x84}, // Z
        {0x90, 0x70, 0x81}, // C
        {0x10, 0xF0, 0x89}, // NC
        {0x81, 0x7F, 0xA1}, // HC
        {0x01, 0xFF, 0xA9}, // HNC
        {0x80, 0x80, 0x8B}, // NVC
    };
    static const int numTests8 = sizeof(tests8) / sizeof(tests8[0]);

    int i = 0;
    unsigned char cc;
    unsigned char result;

    writeString("NEG (8)");

    for(auto &test : tests8)
    {
        asm volatile(
            "neg %2l\n"
            "stc ccr, r0l\n"
            "mov r0, %1\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a)
            : "r0"
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
