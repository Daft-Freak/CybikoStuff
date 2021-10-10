#include <stdio.h>

#include "Registers.hpp"

#include "helpers.h"

// no ldc/stc.b (yet?)

void ldc16Test()
{
    writeString("LDC (16)");

    int res = 1;
    long regs[8];
    unsigned char cc;

    testData[0] = 0x8A000000;
    testData[1] = 0x85868700;
    testData[2] = 0x8F000000;
    stackTestData[0] = 0x8A000000;
    stackTestData[1] = 0x85000000;
    stackTestData[2] = 0x8F000000;

    asm volatile(
        "mov #0, er2\n"

        // indirect
        "mov %[addr], er1\n"
        "ldc @er1, ccr\n"
        "stc ccr, r2l\n"
        CHECK_EQUAL(2, 0x85)

        // ...disp 16
        SET_TEST(2)
        "ldc @(4:16, er1), ccr\n"
        "stc ccr, r2l\n"
        CHECK_EQUAL(2, 0x8F)
        SET_TEST(3)
        "ldc @(-4:16, er1), ccr\n"
        "stc ccr, r2l\n"
        CHECK_EQUAL(2, 0x8A)

        // ...disp 32
        SET_TEST(4)
        "ldc @(4:32, er1), ccr\n"
        "stc ccr, r2l\n"
        CHECK_EQUAL(2, 0x8F)
        SET_TEST(5)
        "ldc @(-4:32, er1), ccr\n"
        "stc ccr, r2l\n"
        CHECK_EQUAL(2, 0x8A)

        // ...inc
        SET_TEST(6)
        "ldc @er1+, ccr\n"
        "stc ccr, r2l\n"
        CHECK_EQUAL(2, 0x85)
        SET_TEST(7)
        "ldc @er1+, ccr\n"
        "stc ccr, r2l\n"
        CHECK_EQUAL(2, 0x87)

        // abs 16
        SET_TEST(8)
        "ldc %[mem16]:16, ccr\n"
        "stc ccr, r2l\n"
        CHECK_EQUAL(2, 0x8A)

        //abs 32
        SET_TEST(9)
        "ldc %[mem32]:32, ccr\n"
        "stc ccr, r2l\n"
        CHECK_EQUAL(2, 0x8A)

        // success
        "mov #0, r0\n"
        "mov r0, %[res]\n"
        "1:\n"
        "stc ccr, %[cc]l\n"
        COPY_REGS
        : [res] "+m" (res), REGS_VARS, [cc] "=r" (cc), [mem32] "+m" (testData), [mem16] "+m" (stackTestData)
        : [addr] "i" (testData + 1)
        : "er0", "er1", "er2", "cc"
    );

    if(res == 0)
        writeString(".\n");
    else
    {
        char buf[50];
        snprintf(buf, sizeof(buf), " failed at %i. ccr: %02x ", res, cc);
        writeString(buf);
        dumpRegs(regs);
        writeString("\n");
    }
}

void stc16Test()
{
    writeString("STC (16)");

    int res = 1;
    long regs[8];
    unsigned char cc;

    testData[0] = 0;
    testData[1] = 0;
    testData[2] = 0;
    stackTestData[0] = 0;
    stackTestData[1] = 0;
    stackTestData[2] = 0;

    asm volatile(
        // indirect
        "mov %[addr], er1\n"
        "ldc #0x80, ccr\n"
        "stc ccr, @er1\n"
        "mov @er1, er2\n"
        CHECK_EQUAL(2, 0x80800000) // second byte from stc is undefined! (seems to be the same as the first though)

        // ...disp 16
        SET_TEST(2)
        "ldc #0x81, ccr\n"
        "stc ccr, @(4:16, er1)\n"
        "mov @(4:16, er1), er2\n"
        CHECK_EQUAL(2, 0x81810000)
        SET_TEST(3)
        "ldc #0x83, ccr\n"
        "stc ccr, @(-4:16, er1)\n"
        "mov @(-4:16, er1), er2\n"
        CHECK_EQUAL(2, 0x83830000)

        // ...disp 32
        SET_TEST(4)
        "ldc #0x85, ccr\n"
        "stc ccr, @(4:32, er1)\n"
        "mov @(4:32, er1), er2\n"
        CHECK_EQUAL(2, 0x85850000)
        SET_TEST(5)
        "ldc #0x87, ccr\n"
        "stc ccr, @(-4:32, er1)\n"
        "mov @(-4:32, er1), er2\n"
        CHECK_EQUAL(2, 0x87870000)

        // ...dec
        SET_TEST(6)
        "ldc #0x82, ccr\n"
        "stc ccr, @-er1\n"
        "mov %[mem32], er2\n"
        CHECK_EQUAL(2, 0x87878282) // leftover junk from previous test
        SET_TEST(7)
        "ldc #0x85, ccr\n"
        "stc ccr, @-er1\n"
        "mov %[mem32], er2\n"
        CHECK_EQUAL(2, 0x85858282)

        // abs 16
        SET_TEST(8)
        "ldc #0x88, ccr\n"
        "stc ccr, %[mem16]:16\n"
        "mov %[mem16]:16, er2\n"
        CHECK_EQUAL(2, 0x88880000)

        //abs 32
        SET_TEST(9)
        "ldc #0x8C, ccr\n"
        "stc ccr, %[mem32]:32\n"
        "mov %[mem32]:32, er2\n"
        CHECK_EQUAL(2, 0x8C8C8282) // junk from indirect dec test

        // success
        "mov #0, r0\n"
        "mov r0, %[res]\n"
        "1:\n"
        "stc ccr, %[cc]l\n"
        COPY_REGS
        : [res] "+m" (res), REGS_VARS, [cc] "=r" (cc), [mem32] "+m" (testData), [mem16] "+m" (stackTestData)
        : [addr] "i" (testData + 1)
        : "er0", "er1", "er2", "cc"
    );

    if(res == 0)
        writeString(".\n");
    else
    {
        char buf[50];
        snprintf(buf, sizeof(buf), " failed at %i. ccr: %02x ", res, cc);
        writeString(buf);
        dumpRegs(regs);
        writeString("\n");
    }
}

void andcTest()
{
    struct TestValue8
    {
        unsigned char a, b, result;
    };

    static const TestValue8 tests8[]
    {
        {0x00, 0x00, 0x00},
        {0x01, 0x01, 0x01},
        {0x80, 0x00, 0x00},
        {0x80, 0x80, 0x80},
        {0xFF, 0xFF, 0xFF},
        {0xAA, 0x55, 0x00},
        {0x37, 0xBF, 0x37},
        {0xFF, 0xAA, 0xAA},
    };
    static const int numTests8 = sizeof(tests8) / sizeof(tests8[0]);

    int i = 0;
    writeString("ANDC");

    unsigned char result;

    // must unroll this loop to use constants
    #pragma GCC unroll 32
    for(auto &test : tests8)
    {
        asm volatile(
            "ldc %1, ccr\n"
            "andc %2, ccr\n"
            "stc ccr, %0l\n"
            : "=r" (result)
            : "i" (test.a), "i" (test.b)
            : "cc"
        );

        if(result != test.result)
            break;

        i++;
    }

    asm volatile("ldc #0x80, ccr");

    if(i < numTests8)
    {
        char buf[60];
        auto &test = tests8[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %02X, got %02X\n", i + 1, test.result, result);
        writeString(buf);
    }
    else
        writeString(".\n");
}

void orcTest()
{
    struct TestValue8
    {
        unsigned char a, b, result;
    };

    static const TestValue8 tests8[]
    {
        {0x00, 0x00, 0x00},
        {0x01, 0x01, 0x01},
        {0x80, 0x00, 0x80},
        {0x80, 0x80, 0x80},
        {0xFF, 0xFF, 0xFF},
        {0xAA, 0x55, 0xFF},
        {0x37, 0xBF, 0xBF},
        {0xFF, 0xAA, 0xFF},
    };
    static const int numTests8 = sizeof(tests8) / sizeof(tests8[0]);

    int i = 0;
    writeString("ORC");

    unsigned char result;

    // must unroll this loop to use constants
    #pragma GCC unroll 32
    for(auto &test : tests8)
    {
        asm volatile(
            "ldc %1, ccr\n"
            "orc %2, ccr\n"
            "stc ccr, %0l\n"
            : "=r" (result)
            : "i" (test.a), "i" (test.b)
            : "cc"
        );

        if(result != test.result)
            break;

        i++;
    }

    asm volatile("ldc #0x80, ccr");

    if(i < numTests8)
    {
        char buf[60];
        auto &test = tests8[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %02X, got %02X\n", i + 1, test.result, result);
        writeString(buf);
    }
    else
        writeString(".\n");
}

void xorcTest()
{
    struct TestValue8
    {
        unsigned char a, b, result;
    };

    static const TestValue8 tests8[]
    {
        {0x00, 0x00, 0x00},
        {0x01, 0x01, 0x00},
        {0x80, 0x00, 0x80},
        {0x80, 0x80, 0x00},
        {0xFF, 0xFF, 0x00},
        {0xAA, 0x55, 0xFF},
        {0x37, 0xBF, 0x88},
        {0xFF, 0xAA, 0x55},
    };
    static const int numTests8 = sizeof(tests8) / sizeof(tests8[0]);

    int i = 0;
    writeString("XORC");

    unsigned char result;

    // must unroll this loop to use constants
    #pragma GCC unroll 32
    for(auto &test : tests8)
    {
        asm volatile(
            "ldc %1, ccr\n"
            "xorc %2, ccr\n"
            "stc ccr, %0l\n"
            : "=r" (result)
            : "i" (test.a), "i" (test.b)
            : "cc"
        );

        if(result != test.result)
            break;

        i++;
    }

    asm volatile("ldc #0x80, ccr");

    if(i < numTests8)
    {
        char buf[60];
        auto &test = tests8[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %02X, got %02X\n", i + 1, test.result, result);
        writeString(buf);
    }
    else
        writeString(".\n");
}
