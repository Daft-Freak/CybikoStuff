#include <stdio.h>

#include "Registers.hpp"

#include "helpers.h"

void daaTest()
{
    struct TestValue8
    {
        unsigned char a, result;
        unsigned char inCC, cc;
    };

    static const TestValue8 tests8[]
    {
        // H/V is unspecified, these values are from running on hardware
        {0x00, 0x00, 0x80, 0x84}, // + 0
        {0x99, 0x99, 0x80, 0x88}, // + 0
        {0x0A, 0x10, 0x80, 0xA0}, // + 0x06
        {0x8F, 0x95, 0x80, 0xA8}, // + 0x06
        {0x00, 0x06, 0xA0, 0x80}, // + 0x06
        {0x93, 0x99, 0xA0, 0x88}, // + 0x06
        {0xA0, 0x00, 0x80, 0x85}, // + 0x60
        {0xF9, 0x59, 0x80, 0x81}, // + 0x60
        {0x9A, 0x00, 0x80, 0xA5}, // + 0x66
        {0xFF, 0x65, 0x80, 0xA1}, // + 0x66
        {0xA0, 0x06, 0xA0, 0x81}, // + 0x66
        {0xF3, 0x59, 0xA0, 0x81}, // + 0x66
        {0x00, 0x60, 0x81, 0x81}, // + 0x60
        {0x29, 0x89, 0x81, 0x8B}, // + 0x60
        {0x0A, 0x70, 0x81, 0x81}, // + 0x66
        {0x2F, 0x95, 0x81, 0x8B}, // + 0x66
        {0x00, 0x66, 0xA1, 0xA1}, // + 0x66
        {0x33, 0x99, 0xA1, 0xAB}, // + 0x66
    };
    static const int numTests8 = sizeof(tests8) / sizeof(tests8[0]);

    int i = 0;
    writeString("DAA");

    unsigned char cc;
    unsigned char result;

    for(auto &test : tests8)
    {
        asm volatile(
            "ldc %3l, ccr\n"
            "daa %2l\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a), "r" (test.inCC)
            :
        );

        // h/v flags have no guaranteed value
        if(result != test.result || (cc & 0x8D) != (test.cc & 0x8D))
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

void dasTest()
{
    struct TestValue8
    {
        unsigned char a, result;
        unsigned char inCC, cc;
    };

    static const TestValue8 tests8[]
    {
        // H/V is unspecified, these values are from running on hardware
        {0x00, 0x00, 0x80, 0x84}, // + 0
        {0x99, 0x99, 0x80, 0x88}, // + 0
        {0x06, 0x00, 0xA0, 0x84}, // - 0x06
        {0x8F, 0x89, 0xA0, 0x88}, // - 0x06
        {0x70, 0x10, 0x81, 0x81}, // - 0x60
        {0xF9, 0x99, 0x81, 0x89}, // - 0x60
        {0x66, 0x00, 0xA1, 0xA5}, // - 0x66
        {0xFF, 0x99, 0xA1, 0x89}, // - 0x66
    };
    static const int numTests8 = sizeof(tests8) / sizeof(tests8[0]);

    int i = 0;
    writeString("DAS");

    unsigned char cc;
    unsigned char result;

    for(auto &test : tests8)
    {
        asm volatile(
            "ldc %3l, ccr\n"
            "das %2l\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a), "r" (test.inCC)
            :
        );

        // h/v flags have no guaranteed value
        if(result != test.result || (cc & 0x8D) != (test.cc & 0x8D))
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
