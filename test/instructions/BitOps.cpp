#include <stdio.h>

#include "Registers.hpp"

#include "helpers.h"

// BSET, BCLR, BNOT, BST, BIST (no flags, value)
#define TEST_BIT_VALUE_I(inst, i, dest, testReg, v1, v2) \
    SET_TEST(i) \
    "ldc #0x80, ccr\n" \
    #inst" #0, "#dest"\n" \
    CHECK_COND_nzvc /* doesn't set flags*/ \
    "mov "#dest ", "#testReg"\n" \
    CHECK_EQUAL(2, v1) \
    \
    SET_TEST(i + 1) \
    "ldc #0x8F, ccr\n" \
    #inst" #7, "#dest"\n" \
    CHECK_COND_NZVC /* ...nope, still no flags*/ \
    "mov "#dest ", "#testReg"\n" \
    CHECK_EQUAL(2, v2)

#define TEST_BIT_VALUE_R(inst, i, dest, testReg, v1, v2) \
    SET_TEST(i) \
    "ldc #0x80, ccr\n" \
    #inst" r1l, "#dest"\n" \
    CHECK_COND_nzvc /* doesn't set flags*/ \
    "mov "#dest ", "#testReg"\n" \
    CHECK_EQUAL(2, v1) \
    \
    SET_TEST(i + 1) \
    "add #0x7, r1l\n" \
    "ldc #0x8F, ccr\n" \
    #inst" r1l, "#dest"\n" \
    CHECK_COND_NZVC /* ...nope, still no flags*/ \
    "mov "#dest ", "#testReg"\n" \
    CHECK_EQUAL(2, v2)

// BTST, BLD, BILD, BAND, BIAND, BOR, BIOR, BXOR, BIXOR (no value, flags)
#define TEST_BIT_COND_I(inst, i, dest, testReg, v, cond1, cond2) \
    SET_TEST(i) \
    "ldc #0x80, ccr\n" \
    #inst" #0, "#dest"\n" \
    CHECK_COND_##cond1 \
    "mov "#dest ", "#testReg"\n" \
    CHECK_EQUAL(2, v) \
    \
    SET_TEST(i + 1) \
    "ldc #0x8F, ccr\n" \
    #inst" #7, "#dest"\n" \
    CHECK_COND_##cond2 \
    "mov "#dest ", "#testReg"\n" \
    CHECK_EQUAL(2, v)

#define TEST_BIT_COND_R(inst, i, dest, testReg, v, cond1, cond2) \
    SET_TEST(i) \
    "ldc #0x80, ccr\n" \
    #inst" r1l, "#dest"\n" \
    CHECK_COND_##cond1 \
    "mov "#dest ", "#testReg"\n" \
    CHECK_EQUAL(2, v) \
    \
    SET_TEST(i + 1) \
    "add #0x7, r1l\n" \
    "ldc #0x8F, ccr\n" \
    #inst" r1l, "#dest"\n" \
    CHECK_COND_##cond2 \
    "mov "#dest ", "#testReg"\n" \
    CHECK_EQUAL(2, v)

void bsetTest()
{
    writeString("BSET");

    int res = 1;
    long regs[8];
    unsigned char cc;

    testData[0] = 0;
    stackTestData[0] = 0;
    TCORA1 = 0;

    asm volatile(
        // imm, direct
        "mov #0, er2\n"
        TEST_BIT_VALUE_I(bset, 1, r2l, r2l, 0x01, 0x81) // results in a useless mov...

        // imm, indirect
        "mov %[addr], er1\n"
        TEST_BIT_VALUE_I(bset, 3, @er1, er2, 0x01000000, 0x81000000)

        // imm, addr 8
        "mov #0, er2\n"
        TEST_BIT_VALUE_I(bset, 5, %[mem8]:8, r2l, 0x01, 0x81)

        // imm, addr 16
        TEST_BIT_VALUE_I(bset, 7, %[mem16]:16, er2, 0x01000000, 0x81000000)

        // reset mem32
        "mov #0, er1\n"
        "mov er1 %[mem32]\n"

        // imm, addr 32
        TEST_BIT_VALUE_I(bset, 9, %[mem32]:32, er2, 0x01000000, 0x81000000)

        // reset
        "mov #0, er2\n"
        "mov er2 %[mem32]\n"
        "mov er2 %[mem16]\n"
        "mov r2l %[mem8]\n"

        // reg tests, also checks that only the low 3 bits are used

        // reg, direct
        "mov #0, r1l\n"
        TEST_BIT_VALUE_R(bset, 11, r2l, r2l, 0x01, 0x81)

        // reg, indirect
        "mov #8, r1l\n"
        "mov %[addr], er3\n"
        TEST_BIT_VALUE_R(bset, 13, @er3, er2, 0x01000000, 0x81000000)

        // reg, addr 8
        "mov #0, er2\n"
        "mov #0x18, r1l\n"
        TEST_BIT_VALUE_R(bset, 15, %[mem8]:8, r2l, 0x01, 0x81)

        // reg, addr 16
        "mov #0x38, r1l\n"
        TEST_BIT_VALUE_R(bset, 17, %[mem16]:16, er2, 0x01000000, 0x81000000)

        // reset mem32
        "mov #0, er1\n"
        "mov er1 %[mem32]\n"

        // reg, addr 32
        "mov #0x78, r1l\n"
        TEST_BIT_VALUE_R(bset, 19, %[mem32]:32, er2, 0x01000000, 0x81000000)

        // success
        "mov #0, r0\n"
        "mov r0, %[res]\n"
        "1:\n"
        "stc ccr, %[cc]l\n"
        COPY_REGS
        : [res] "+m" (res), REGS_VARS, [cc] "=r" (cc), [mem32] "+m" (testData), [mem16] "+m" (stackTestData), [mem8] "+m" (TCORA1)
        : [addr] "i" (testData)
        : "er0", "er1", "er2", "er3", "cc"
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

void bclrTest()
{
    writeString("BCLR");

    int res = 1;
    long regs[8];
    unsigned char cc;

    testData[0] = 0xFFFFFFFF;
    stackTestData[0] = 0xFFFFFFFF;
    TCORA1 = 0xFF;

    asm volatile(
        // imm, direct
        "mov #0xFFFFFFFF, er2\n"
        TEST_BIT_VALUE_I(bclr, 1, r2l, r2l, 0xFFFFFFFE, 0xFFFFFF7E)

        // imm, indirect
        "mov %[addr], er1\n"
        TEST_BIT_VALUE_I(bclr, 3, @er1, er2, 0xFEFFFFFF, 0x7EFFFFFF)

        // imm, addr 8
        "mov #0, er2\n"
        TEST_BIT_VALUE_I(bclr, 5, %[mem8]:8, r2l, 0xFE, 0x7E)

        // imm, addr 16
        TEST_BIT_VALUE_I(bclr, 7, %[mem16]:16, er2, 0xFEFFFFFF, 0x7EFFFFFF)

        // reset mem32
        "mov #0xFFFFFFFF, er1\n"
        "mov er1 %[mem32]\n"

        // imm, addr 32
        TEST_BIT_VALUE_I(bclr, 9, %[mem32]:32, er2, 0xFEFFFFFF, 0x7EFFFFFF)

        // reset
        "mov #0xFFFFFFFF, er2\n"
        "mov er2 %[mem32]\n"
        "mov er2 %[mem16]\n"
        "mov r2l %[mem8]\n"

        // reg tests, also checks that only the low 3 bits are used

        // reg, direct
        "mov #0, r1l\n"
        TEST_BIT_VALUE_R(bclr, 11, r2l, r2l, 0xFFFFFFFE, 0xFFFFFF7E)

        // reg, indirect
        "mov #8, r1l\n"
        "mov %[addr], er3\n"
        TEST_BIT_VALUE_R(bclr, 13, @er3, er2, 0xFEFFFFFF, 0x7EFFFFFF)

        // reg, addr 8
        "mov #0, er2\n"
        "mov #0x18, r1l\n"
        TEST_BIT_VALUE_R(bclr, 15, %[mem8]:8, r2l, 0xFE, 0x7E)

        // reg, addr 16
        "mov #0x38, r1l\n"
        TEST_BIT_VALUE_R(bclr, 17, %[mem16]:16, er2, 0xFEFFFFFF, 0x7EFFFFFF)

        // reset mem32
        "mov #0xFFFFFFFF, er1\n"
        "mov er1 %[mem32]\n"

        // reg, addr 32
        "mov #0x78, r1l\n"
        TEST_BIT_VALUE_R(bclr, 19, %[mem32]:32, er2, 0xFEFFFFFF, 0x7EFFFFFF)

        // success
        "mov #0, r0\n"
        "mov r0, %[res]\n"
        "1:\n"
        "stc ccr, %[cc]l\n"
        COPY_REGS
        : [res] "+m" (res), REGS_VARS, [cc] "=r" (cc), [mem32] "+m" (testData), [mem16] "+m" (stackTestData), [mem8] "+m" (TCORA1)
        : [addr] "i" (testData)
        : "er0", "er1", "er2", "er3", "cc"
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

void bnotTest()
{
    writeString("BNOT");

    int res = 1;
    long regs[8];
    unsigned char cc;

    testData[0] = 0;
    stackTestData[0] = 0;
    TCORA1 = 0;

    // this one is longer as it flips the bits back

    asm volatile(
        // imm, direct
        "mov #0, er2\n"
        TEST_BIT_VALUE_I(bnot, 1, r2l, r2l, 0x01, 0x81) // results in a useless mov...

        // flip them back
        SET_TEST(3)
        "bnot #0, r2l\n"
        "bnot #7, r2l\n"
        CHECK_EQUAL(2, 0)

        // imm, indirect
        "mov %[addr], er1\n"
        TEST_BIT_VALUE_I(bnot, 4, @er1, er2, 0x01000000, 0x81000000)

        SET_TEST(6)
        "bnot #0, @er1\n"
        "bnot #7, @er1\n"
        "mov @er1, er2\n"
        CHECK_EQUAL(2, 0)

        // imm, addr 8
        "mov #0, er2\n"
        TEST_BIT_VALUE_I(bnot, 7, %[mem8]:8, r2l, 0x01, 0x81)

        SET_TEST(9)
        "bnot #0, %[mem8]:8\n"
        "bnot #7, %[mem8]:8\n"
        "mov %[mem8]:8, r2l\n"
        CHECK_EQUAL(2, 0)

        // imm, addr 16
        TEST_BIT_VALUE_I(bnot, 10, %[mem16]:16, er2, 0x01000000, 0x81000000)

        SET_TEST(12)
        "bnot #0, %[mem16]:16\n"
        "bnot #7, %[mem16]:16\n"
        "mov %[mem16]:16, er2\n"
        CHECK_EQUAL(2, 0)

        // imm, addr 32
        TEST_BIT_VALUE_I(bnot, 13, %[mem32]:32, er2, 0x01000000, 0x81000000)

        SET_TEST(15)
        "bnot #0, %[mem32]:32\n"
        "bnot #7, %[mem32]:32\n"
        "mov %[mem32]:32, er2\n"
        CHECK_EQUAL(2, 0)

        // reg tests, also checks that only the low 3 bits are used

        // reg, direct
        "mov #0, r1l\n"
        TEST_BIT_VALUE_R(bnot, 16, r2l, r2l, 0x01, 0x81)

        SET_TEST(18)
        "bnot r1l, r2l\n"
        "mov #0, r1l\n"
        "bnot r1l, r2l\n"
        CHECK_EQUAL(2, 0)

        // reg, indirect
        "mov #8, r1l\n"
        "mov %[addr], er3\n"
        TEST_BIT_VALUE_R(bnot, 19, @er3, er2, 0x01000000, 0x81000000)

        SET_TEST(21)
        "bnot r1l, @er3\n"
        "mov #0, r1l\n"
        "bnot r1l, @er3\n"
        "mov @er3, er2\n"
        CHECK_EQUAL(2, 0)

        // reg, addr 8
        "mov #0, er2\n"
        "mov #0x18, r1l\n"
        TEST_BIT_VALUE_R(bnot, 22, %[mem8]:8, r2l, 0x01, 0x81)

        SET_TEST(24)
        "bnot r1l, %[mem8]:8\n"
        "mov #0, r1l\n"
        "bnot r1l %[mem8]:8\n"
        "mov %[mem8]:8, r2l\n"
        CHECK_EQUAL(2, 0)

        // reg, addr 16
        "mov #0x38, r1l\n"
        TEST_BIT_VALUE_R(bnot, 25, %[mem16]:16, er2, 0x01000000, 0x81000000)

        SET_TEST(27)
        "bnot r1l, %[mem16]:16\n"
        "mov #0, r1l\n"
        "bnot r1l %[mem16]:16\n"
        "mov %[mem16]:16, er2\n"
        CHECK_EQUAL(2, 0)

        // reg, addr 32
        "mov #0x78, r1l\n"
        TEST_BIT_VALUE_R(bnot, 28, %[mem32]:32, er2, 0x01000000, 0x81000000)

        SET_TEST(30)
        "bnot r1l, %[mem32]:32\n"
        "mov #0, r1l\n"
        "bnot r1l %[mem32]:32\n"
        "mov %[mem32]:32, er2\n"
        CHECK_EQUAL(2, 0)

        // success
        "mov #0, r0\n"
        "mov r0, %[res]\n"
        "1:\n"
        "stc ccr, %[cc]l\n"
        COPY_REGS
        : [res] "+m" (res), REGS_VARS, [cc] "=r" (cc), [mem32] "+m" (testData), [mem16] "+m" (stackTestData), [mem8] "+m" (TCORA1)
        : [addr] "i" (testData)
        : "er0", "er1", "er2", "er3", "cc"
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

void btstTest()
{
    writeString("BTST");

    int res = 1;
    long regs[8];
    unsigned char cc;

    testData[0] = 0xAAAAAAAA;
    stackTestData[0] = 0xAAAAAAAA;
    TCORA1 = 0xAA;

    asm volatile(
        // imm, direct
        "mov #0xAA, er2\n"
        TEST_BIT_COND_I(btst, 1, r2l, r2l, 0xAA, nZvc, NzVC)

        // imm, indirect
        "mov %[addr], er1\n"
        TEST_BIT_COND_I(btst, 3, @er1, er2, 0xAAAAAAAA, nZvc, NzVC)

        // imm, addr 8
        "mov #0, er2\n"
        TEST_BIT_COND_I(btst, 5, %[mem8]:8, r2l, 0xAA, nZvc, NzVC)

        // imm, addr 16
        TEST_BIT_COND_I(btst, 7, %[mem16]:16, er2, 0xAAAAAAAA, nZvc, NzVC)

        // imm, addr 32
        TEST_BIT_COND_I(btst, 9, %[mem32]:32, er2, 0xAAAAAAAA, nZvc, NzVC)

        // reg tests, also checks that only the low 3 bits are used

        // reg, direct
        "mov #0xAA, er2\n"
        "mov #0, r1l\n"
        TEST_BIT_COND_R(btst, 11, r2l, r2l, 0xAA, nZvc, NzVC)

        // reg, indirect
        "mov #8, r1l\n"
        "mov %[addr], er3\n"
        TEST_BIT_COND_R(btst, 13, @er3, er2, 0xAAAAAAAA, nZvc, NzVC)

        // reg, addr 8
        "mov #0, er2\n"
        "mov #0x18, r1l\n"
        TEST_BIT_COND_R(btst, 15, %[mem8]:8, r2l, 0xAA, nZvc, NzVC)

        // reg, addr 16
        "mov #0x38, r1l\n"
        TEST_BIT_COND_R(btst, 17, %[mem16]:16, er2, 0xAAAAAAAA, nZvc, NzVC)

        // reg, addr 32
        "mov #0x78, r1l\n"
        TEST_BIT_COND_R(btst, 19, %[mem32]:32, er2, 0xAAAAAAAA, nZvc, NzVC)

        // success
        "mov #0, r0\n"
        "mov r0, %[res]\n"
        "1:\n"
        "stc ccr, %[cc]l\n"
        COPY_REGS
        : [res] "+m" (res), REGS_VARS, [cc] "=r" (cc), [mem32] "+m" (testData), [mem16] "+m" (stackTestData), [mem8] "+m" (TCORA1)
        : [addr] "i" (testData)
        : "er0", "er1", "er2", "er3", "cc"
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

void bldTest()
{
    writeString("BLD");

    int res = 1;
    long regs[8];
    unsigned char cc;

    testData[0] = 0x55555555;
    stackTestData[0] = 0x55555555;
    TCORA1 = 0x55;

    asm volatile(
        // imm, direct
        "mov #0x55, er2\n"
        TEST_BIT_COND_I(bld, 1, r2l, r2l, 0x55, nzvC, NZVc)

        // imm, indirect
        "mov %[addr], er1\n"
        TEST_BIT_COND_I(bld, 3, @er1, er2, 0x55555555, nzvC, NZVc)

        // imm, addr 8
        "mov #0, er2\n"
        TEST_BIT_COND_I(bld, 5, %[mem8]:8, r2l, 0x55, nzvC, NZVc)

        // imm, addr 16
        TEST_BIT_COND_I(bld, 7, %[mem16]:16, er2, 0x55555555, nzvC, NZVc)

        // imm, addr 32
        TEST_BIT_COND_I(bld, 9, %[mem32]:32, er2, 0x55555555, nzvC, NZVc)

        // success
        "mov #0, r0\n"
        "mov r0, %[res]\n"
        "1:\n"
        "stc ccr, %[cc]l\n"
        COPY_REGS
        : [res] "+m" (res), REGS_VARS, [cc] "=r" (cc), [mem32] "+m" (testData), [mem16] "+m" (stackTestData), [mem8] "+m" (TCORA1)
        : [addr] "i" (testData)
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

void bildTest()
{
    writeString("BILD");

    int res = 1;
    long regs[8];
    unsigned char cc;

    testData[0] = 0xAAAAAAAA;
    stackTestData[0] = 0xAAAAAAAA;
    TCORA1 = 0xAA;

    asm volatile(
        // imm, direct
        "mov #0xAA, er2\n"
        TEST_BIT_COND_I(bild, 1, r2l, r2l, 0xAA, nzvC, NZVc)

        // imm, indirect
        "mov %[addr], er1\n"
        TEST_BIT_COND_I(bild, 3, @er1, er2, 0xAAAAAAAA, nzvC, NZVc)

        // imm, addr 8
        "mov #0, er2\n"
        TEST_BIT_COND_I(bild, 5, %[mem8]:8, r2l, 0xAA, nzvC, NZVc)

        // imm, addr 16
        TEST_BIT_COND_I(bild, 7, %[mem16]:16, er2, 0xAAAAAAAA, nzvC, NZVc)

        // imm, addr 32
        TEST_BIT_COND_I(bild, 9, %[mem32]:32, er2, 0xAAAAAAAA, nzvC, NZVc)

        // success
        "mov #0, r0\n"
        "mov r0, %[res]\n"
        "1:\n"
        "stc ccr, %[cc]l\n"
        COPY_REGS
        : [res] "+m" (res), REGS_VARS, [cc] "=r" (cc), [mem32] "+m" (testData), [mem16] "+m" (stackTestData), [mem8] "+m" (TCORA1)
        : [addr] "i" (testData)
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

void bstTest()
{
    writeString("BST");

    int res = 1;
    long regs[8];
    unsigned char cc;

    testData[0] = 0x0F0F0F0F;
    stackTestData[0] = 0x0F0F0F0F;
    TCORA1 = 0x0F;

    asm volatile(
        // imm, direct
        "mov #0x0F, er2\n"
        TEST_BIT_VALUE_I(bst, 1, r2l, r2l, 0x0E, 0x8E)

        // imm, indirect
        "mov %[addr], er1\n"
        TEST_BIT_VALUE_I(bst, 3, @er1, er2, 0x0E0F0F0F, 0x8E0F0F0F)

        // imm, addr 8
        "mov #0, er2\n"
        TEST_BIT_VALUE_I(bst, 5, %[mem8]:8, r2l, 0x0E, 0x8E)

        // imm, addr 16
        TEST_BIT_VALUE_I(bst, 7, %[mem16]:16, er2, 0x0E0F0F0F, 0x8E0F0F0F)

        // reset mem32
        "mov #0x0F0F0F0F, er1\n"
        "mov er1 %[mem32]\n"

        // imm, addr 32
        TEST_BIT_VALUE_I(bst, 9, %[mem32]:32, er2, 0x0E0F0F0F, 0x8E0F0F0F)

        // success
        "mov #0, r0\n"
        "mov r0, %[res]\n"
        "1:\n"
        "stc ccr, %[cc]l\n"
        COPY_REGS
        : [res] "+m" (res), REGS_VARS, [cc] "=r" (cc), [mem32] "+m" (testData), [mem16] "+m" (stackTestData), [mem8] "+m" (TCORA1)
        : [addr] "i" (testData)
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

void bistTest()
{
    writeString("BIST");

    int res = 1;
    long regs[8];
    unsigned char cc;

    testData[0] = 0xF0F0F0F0;
    stackTestData[0] = 0xF0F0F0F0;
    TCORA1 = 0xF0;

    asm volatile(
        // imm, direct
        "mov #0xF0, er2\n"
        TEST_BIT_VALUE_I(bist, 1, r2l, r2l, 0xF1, 0x71)

        // imm, indirect
        "mov %[addr], er1\n"
        TEST_BIT_VALUE_I(bist, 3, @er1, er2, 0xF1F0F0F0, 0x71F0F0F0)

        // imm, addr 8
        "mov #0, er2\n"
        TEST_BIT_VALUE_I(bist, 5, %[mem8]:8, r2l, 0xF1, 0x71)

        // imm, addr 16
        TEST_BIT_VALUE_I(bist, 7, %[mem16]:16, er2, 0xF1F0F0F0, 0x71F0F0F0)

        // reset mem32
        "mov #0xF0F0F0F0, er1\n"
        "mov er1 %[mem32]\n"

        // imm, addr 32
        TEST_BIT_VALUE_I(bist, 9, %[mem32]:32, er2, 0xF1F0F0F0, 0x71F0F0F0)

        // success
        "mov #0, r0\n"
        "mov r0, %[res]\n"
        "1:\n"
        "stc ccr, %[cc]l\n"
        COPY_REGS
        : [res] "+m" (res), REGS_VARS, [cc] "=r" (cc), [mem32] "+m" (testData), [mem16] "+m" (stackTestData), [mem8] "+m" (TCORA1)
        : [addr] "i" (testData)
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

void bandTest()
{
    writeString("BAND");

    int res = 1;
    long regs[8];
    unsigned char cc;

    testData[0] = 0x55555555;
    stackTestData[0] = 0x55555555;
    TCORA1 = 0x55;

    asm volatile(
        // imm, direct
        "mov #0x55, er2\n"
        TEST_BIT_COND_I(band, 1, r2l, r2l, 0x55, nzvc, NZVc)
        "mov #0xAA, er2\n"
        TEST_BIT_COND_I(band, 3, r2l, r2l, 0xAA, nzvc, NZVC)

        // imm, indirect
        "mov %[addr], er1\n"
        TEST_BIT_COND_I(band, 5, @er1, er2, 0x55555555, nzvc, NZVc)
        "mov #0xAAAAAAAA, er2\n"
        "mov er2, @er1\n"
        TEST_BIT_COND_I(band, 7, @er1, er2, 0xAAAAAAAA, nzvc, NZVC)

        // imm, addr 8
        "mov #0, er2\n"
        TEST_BIT_COND_I(band, 9, %[mem8]:8, r2l, 0x55, nzvc, NZVc)
        "mov #0xAA, r1l\n"
        "mov r1l, %[mem8]:8\n"
        TEST_BIT_COND_I(band, 11, %[mem8]:8, r2l, 0xAA, nzvc, NZVC)

        // imm, addr 16
        TEST_BIT_COND_I(band, 13, %[mem16]:16, er2, 0x55555555, nzvc, NZVc)
        "mov #0xAAAAAAAA, er2\n"
        "mov er2, %[mem16]:16\n"
        TEST_BIT_COND_I(band, 13, %[mem16]:16, er2, 0xAAAAAAAA, nzvc, NZVC)

        // imm, addr 32
        "mov #0x55555555, er2\n"
        "mov er2, %[mem32]:32\n"
        TEST_BIT_COND_I(band, 17, %[mem32]:32, er2, 0x55555555, nzvc, NZVc)
        "mov #0xAAAAAAAA, er2\n"
        "mov er2, %[mem32]:32\n"
        TEST_BIT_COND_I(band, 19, %[mem16]:32, er2, 0xAAAAAAAA, nzvc, NZVC)

        // success
        "mov #0, r0\n"
        "mov r0, %[res]\n"
        "1:\n"
        "stc ccr, %[cc]l\n"
        COPY_REGS
        : [res] "+m" (res), REGS_VARS, [cc] "=r" (cc), [mem32] "+m" (testData), [mem16] "+m" (stackTestData), [mem8] "+m" (TCORA1)
        : [addr] "i" (testData)
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

void biandTest()
{
    writeString("BIAND");

    int res = 1;
    long regs[8];
    unsigned char cc;

    testData[0] = 0x55555555;
    stackTestData[0] = 0x55555555;
    TCORA1 = 0x55;

    asm volatile(
        // imm, direct
        "mov #0x55, er2\n"
        TEST_BIT_COND_I(biand, 1, r2l, r2l, 0x55, nzvc, NZVC)
        "mov #0xAA, er2\n"
        TEST_BIT_COND_I(biand, 3, r2l, r2l, 0xAA, nzvc, NZVc)

        // imm, indirect
        "mov %[addr], er1\n"
        TEST_BIT_COND_I(biand, 5, @er1, er2, 0x55555555, nzvc, NZVC)
        "mov #0xAAAAAAAA, er2\n"
        "mov er2, @er1\n"
        TEST_BIT_COND_I(biand, 7, @er1, er2, 0xAAAAAAAA, nzvc, NZVc)

        // imm, addr 8
        "mov #0, er2\n"
        TEST_BIT_COND_I(biand, 9, %[mem8]:8, r2l, 0x55, nzvc, NZVC)
        "mov #0xAA, r1l\n"
        "mov r1l, %[mem8]:8\n"
        TEST_BIT_COND_I(biand, 11, %[mem8]:8, r2l, 0xAA, nzvc, NZVc)

        // imm, addr 16
        TEST_BIT_COND_I(biand, 13, %[mem16]:16, er2, 0x55555555, nzvc, NZVC)
        "mov #0xAAAAAAAA, er2\n"
        "mov er2, %[mem16]:16\n"
        TEST_BIT_COND_I(biand, 13, %[mem16]:16, er2, 0xAAAAAAAA, nzvc, NZVc)

        // imm, addr 32
        "mov #0x55555555, er2\n"
        "mov er2, %[mem32]:32\n"
        TEST_BIT_COND_I(biand, 17, %[mem32]:32, er2, 0x55555555, nzvc, NZVC)
        "mov #0xAAAAAAAA, er2\n"
        "mov er2, %[mem32]:32\n"
        TEST_BIT_COND_I(biand, 19, %[mem16]:32, er2, 0xAAAAAAAA, nzvc, NZVc)

        // success
        "mov #0, r0\n"
        "mov r0, %[res]\n"
        "1:\n"
        "stc ccr, %[cc]l\n"
        COPY_REGS
        : [res] "+m" (res), REGS_VARS, [cc] "=r" (cc), [mem32] "+m" (testData), [mem16] "+m" (stackTestData), [mem8] "+m" (TCORA1)
        : [addr] "i" (testData)
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

void borTest()
{
    writeString("BOR");

    int res = 1;
    long regs[8];
    unsigned char cc;

    testData[0] = 0x55555555;
    stackTestData[0] = 0x55555555;
    TCORA1 = 0x55;

    asm volatile(
        // imm, direct
        "mov #0x55, er2\n"
        TEST_BIT_COND_I(bor, 1, r2l, r2l, 0x55, nzvC, NZVC)
        "mov #0xAA, er2\n"
        TEST_BIT_COND_I(bor, 3, r2l, r2l, 0xAA, nzvc, NZVC)

        // imm, indirect
        "mov %[addr], er1\n"
        TEST_BIT_COND_I(bor, 5, @er1, er2, 0x55555555, nzvC, NZVC)
        "mov #0xAAAAAAAA, er2\n"
        "mov er2, @er1\n"
        TEST_BIT_COND_I(bor, 7, @er1, er2, 0xAAAAAAAA, nzvc, NZVC)

        // imm, addr 8
        "mov #0, er2\n"
        TEST_BIT_COND_I(bor, 9, %[mem8]:8, r2l, 0x55, nzvC, NZVC)
        "mov #0xAA, r1l\n"
        "mov r1l, %[mem8]:8\n"
        TEST_BIT_COND_I(bor, 11, %[mem8]:8, r2l, 0xAA, nzvc, NZVC)

        // imm, addr 16
        TEST_BIT_COND_I(bor, 13, %[mem16]:16, er2, 0x55555555, nzvC, NZVC)
        "mov #0xAAAAAAAA, er2\n"
        "mov er2, %[mem16]:16\n"
        TEST_BIT_COND_I(bor, 13, %[mem16]:16, er2, 0xAAAAAAAA, nzvc, NZVC)

        // imm, addr 32
        "mov #0x55555555, er2\n"
        "mov er2, %[mem32]:32\n"
        TEST_BIT_COND_I(bor, 17, %[mem32]:32, er2, 0x55555555, nzvC, NZVC)
        "mov #0xAAAAAAAA, er2\n"
        "mov er2, %[mem32]:32\n"
        TEST_BIT_COND_I(bor, 19, %[mem16]:32, er2, 0xAAAAAAAA, nzvc, NZVC)

        // success
        "mov #0, r0\n"
        "mov r0, %[res]\n"
        "1:\n"
        "stc ccr, %[cc]l\n"
        COPY_REGS
        : [res] "+m" (res), REGS_VARS, [cc] "=r" (cc), [mem32] "+m" (testData), [mem16] "+m" (stackTestData), [mem8] "+m" (TCORA1)
        : [addr] "i" (testData)
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

void biorTest()
{
    writeString("BIOR");

    int res = 1;
    long regs[8];
    unsigned char cc;

    testData[0] = 0x55555555;
    stackTestData[0] = 0x55555555;
    TCORA1 = 0x55;

    asm volatile(
        // imm, direct
        "mov #0x55, er2\n"
        TEST_BIT_COND_I(bior, 1, r2l, r2l, 0x55, nzvc, NZVC)
        "mov #0xAA, er2\n"
        TEST_BIT_COND_I(bior, 3, r2l, r2l, 0xAA, nzvC, NZVC)

        // imm, indirect
        "mov %[addr], er1\n"
        TEST_BIT_COND_I(bior, 5, @er1, er2, 0x55555555, nzvc, NZVC)
        "mov #0xAAAAAAAA, er2\n"
        "mov er2, @er1\n"
        TEST_BIT_COND_I(bior, 7, @er1, er2, 0xAAAAAAAA, nzvC, NZVC)

        // imm, addr 8
        "mov #0, er2\n"
        TEST_BIT_COND_I(bior, 9, %[mem8]:8, r2l, 0x55, nzvc, NZVC)
        "mov #0xAA, r1l\n"
        "mov r1l, %[mem8]:8\n"
        TEST_BIT_COND_I(bior, 11, %[mem8]:8, r2l, 0xAA, nzvC, NZVC)

        // imm, addr 16
        TEST_BIT_COND_I(bior, 13, %[mem16]:16, er2, 0x55555555, nzvc, NZVC)
        "mov #0xAAAAAAAA, er2\n"
        "mov er2, %[mem16]:16\n"
        TEST_BIT_COND_I(bior, 13, %[mem16]:16, er2, 0xAAAAAAAA, nzvC, NZVC)

        // imm, addr 32
        "mov #0x55555555, er2\n"
        "mov er2, %[mem32]:32\n"
        TEST_BIT_COND_I(bior, 17, %[mem32]:32, er2, 0x55555555, nzvc, NZVC)
        "mov #0xAAAAAAAA, er2\n"
        "mov er2, %[mem32]:32\n"
        TEST_BIT_COND_I(bior, 19, %[mem16]:32, er2, 0xAAAAAAAA, nzvC, NZVC)

        // success
        "mov #0, r0\n"
        "mov r0, %[res]\n"
        "1:\n"
        "stc ccr, %[cc]l\n"
        COPY_REGS
        : [res] "+m" (res), REGS_VARS, [cc] "=r" (cc), [mem32] "+m" (testData), [mem16] "+m" (stackTestData), [mem8] "+m" (TCORA1)
        : [addr] "i" (testData)
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

void bxorTest()
{
    writeString("BXOR");

    int res = 1;
    long regs[8];
    unsigned char cc;

    testData[0] = 0x55555555;
    stackTestData[0] = 0x55555555;
    TCORA1 = 0x55;

    asm volatile(
        // imm, direct
        "mov #0x55, er2\n"
        TEST_BIT_COND_I(bxor, 1, r2l, r2l, 0x55, nzvC, NZVC)
        "mov #0xAA, er2\n"
        TEST_BIT_COND_I(bxor, 3, r2l, r2l, 0xAA, nzvc, NZVc)

        // imm, indirect
        "mov %[addr], er1\n"
        TEST_BIT_COND_I(bxor, 5, @er1, er2, 0x55555555, nzvC, NZVC)
        "mov #0xAAAAAAAA, er2\n"
        "mov er2, @er1\n"
        TEST_BIT_COND_I(bxor, 7, @er1, er2, 0xAAAAAAAA, nzvc, NZVc)

        // imm, addr 8
        "mov #0, er2\n"
        TEST_BIT_COND_I(bxor, 9, %[mem8]:8, r2l, 0x55, nzvC, NZVC)
        "mov #0xAA, r1l\n"
        "mov r1l, %[mem8]:8\n"
        TEST_BIT_COND_I(bxor, 11, %[mem8]:8, r2l, 0xAA, nzvc, NZVc)

        // imm, addr 16
        TEST_BIT_COND_I(bxor, 13, %[mem16]:16, er2, 0x55555555, nzvC, NZVC)
        "mov #0xAAAAAAAA, er2\n"
        "mov er2, %[mem16]:16\n"
        TEST_BIT_COND_I(bxor, 13, %[mem16]:16, er2, 0xAAAAAAAA, nzvc, NZVc)

        // imm, addr 32
        "mov #0x55555555, er2\n"
        "mov er2, %[mem32]:32\n"
        TEST_BIT_COND_I(bxor, 17, %[mem32]:32, er2, 0x55555555, nzvC, NZVC)
        "mov #0xAAAAAAAA, er2\n"
        "mov er2, %[mem32]:32\n"
        TEST_BIT_COND_I(bxor, 19, %[mem16]:32, er2, 0xAAAAAAAA, nzvc, NZVc)

        // success
        "mov #0, r0\n"
        "mov r0, %[res]\n"
        "1:\n"
        "stc ccr, %[cc]l\n"
        COPY_REGS
        : [res] "+m" (res), REGS_VARS, [cc] "=r" (cc), [mem32] "+m" (testData), [mem16] "+m" (stackTestData), [mem8] "+m" (TCORA1)
        : [addr] "i" (testData)
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

void bixorTest()
{
    writeString("BIXOR");

    int res = 1;
    long regs[8];
    unsigned char cc;

    testData[0] = 0x55555555;
    stackTestData[0] = 0x55555555;
    TCORA1 = 0x55;

    asm volatile(
        // imm, direct
        "mov #0x55, er2\n"
        TEST_BIT_COND_I(bixor, 1, r2l, r2l, 0x55, nzvc, NZVc)
        "mov #0xAA, er2\n"
        TEST_BIT_COND_I(bixor, 3, r2l, r2l, 0xAA, nzvC, NZVC)

        // imm, indirect
        "mov %[addr], er1\n"
        TEST_BIT_COND_I(bixor, 5, @er1, er2, 0x55555555, nzvc, NZVc)
        "mov #0xAAAAAAAA, er2\n"
        "mov er2, @er1\n"
        TEST_BIT_COND_I(bixor, 7, @er1, er2, 0xAAAAAAAA, nzvC, NZVC)

        // imm, addr 8
        "mov #0, er2\n"
        TEST_BIT_COND_I(bixor, 9, %[mem8]:8, r2l, 0x55, nzvc, NZVc)
        "mov #0xAA, r1l\n"
        "mov r1l, %[mem8]:8\n"
        TEST_BIT_COND_I(bixor, 11, %[mem8]:8, r2l, 0xAA, nzvC, NZVC)

        // imm, addr 16
        TEST_BIT_COND_I(bixor, 13, %[mem16]:16, er2, 0x55555555, nzvc, NZVc)
        "mov #0xAAAAAAAA, er2\n"
        "mov er2, %[mem16]:16\n"
        TEST_BIT_COND_I(bixor, 13, %[mem16]:16, er2, 0xAAAAAAAA, nzvC, NZVC)

        // imm, addr 32
        "mov #0x55555555, er2\n"
        "mov er2, %[mem32]:32\n"
        TEST_BIT_COND_I(bixor, 17, %[mem32]:32, er2, 0x55555555, nzvc, NZVc)
        "mov #0xAAAAAAAA, er2\n"
        "mov er2, %[mem32]:32\n"
        TEST_BIT_COND_I(bixor, 19, %[mem16]:32, er2, 0xAAAAAAAA, nzvC, NZVC)

        // success
        "mov #0, r0\n"
        "mov r0, %[res]\n"
        "1:\n"
        "stc ccr, %[cc]l\n"
        COPY_REGS
        : [res] "+m" (res), REGS_VARS, [cc] "=r" (cc), [mem32] "+m" (testData), [mem16] "+m" (stackTestData), [mem8] "+m" (TCORA1)
        : [addr] "i" (testData)
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
