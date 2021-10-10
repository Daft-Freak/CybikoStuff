#include <stdio.h>

#include "Registers.hpp"

#include "helpers.h"

void ldmTest()
{
    writeString("LDM");

    int res = 1;
    long regs[8];
    //char cc;


    asm volatile(
        "mov er7, %[mem]\n" // save stack in mem (we're about to use every register)

        // push some stuff
        //"mov #0x00112233, er2\n"
        //"push er2\n"
        //"mov #0x44556677, er2\n"
        //"push er2\n"
        "mov #0x8899AABB, er2\n"
        "push er2\n"
        "mov #0xCCDDEEFF, er2\n"
        "push er2\n"
        "mov #0xFFEEDDCC, er2\n"
        "push er2\n"
        "mov #0xBBAA9988, er2\n"
        "push er2\n"
        "mov #0x77665544, er2\n"
        "push er2\n"
        "mov #0x33221100, er2\n"
        "push er2\n"

        // now use ldm
        // make sure flags are not affected
        "ldc #0x80, ccr\n"
        "ldm @er7+, er0-er1\n"
        CHECK_COND_nzvc
        CHECK_EQUAL(0, 0x77665544)
        CHECK_EQUAL(1, 0x33221100)
        "ldc #0x8F, ccr\n"
        "ldm @er7+, er2-er3\n"
        CHECK_COND_NZVC
        CHECK_EQUAL(2, 0xFFEEDDCC)
        CHECK_EQUAL(3, 0xBBAA9988)
        "ldm @er7+, er4-er5\n"
        CHECK_EQUAL(4, 0x8899AABB)
        CHECK_EQUAL(5, 0xCCDDEEFF)

        //"ldm @er7+, er6-er7\n" // not valid
        //CHECK_EQUAL(6, 0x00112233)
        //CHECK_EQUAL(7, 0x44556677)

        // make sure stack is where we started
        "mov %[mem], er1\n"
        "cmp er1, er7\n"
        "bne 1f\n"

        // three at a time
        SET_TEST(2)
        "mov #0x00112233, er2\n"
        "push er2\n"
        "mov #0x44556677, er2\n"
        "push er2\n"
        "mov #0x8899AABB, er2\n"
        "push er2\n"
        "mov #0xCCDDEEFF, er2\n"
        "push er2\n"
        "mov #0xFFEEDDCC, er2\n"
        "push er2\n"
        "mov #0xBBAA9988, er2\n"
        "push er2\n"

        "ldc #0x80, ccr\n"
        "ldm @er7+, er0-er2\n"
        CHECK_COND_nzvc
        CHECK_EQUAL(0, 0xCCDDEEFF)
        CHECK_EQUAL(1, 0xFFEEDDCC)
        CHECK_EQUAL(2, 0xBBAA9988)

        "ldc #0x8F, ccr\n"
        "ldm @er7+, er4-er6\n"
        CHECK_COND_NZVC
        CHECK_EQUAL(4, 0x00112233)
        CHECK_EQUAL(5, 0x44556677)
        CHECK_EQUAL(6, 0x8899AABB)

        // make sure stack is where we started
        "mov %[mem], er1\n"
        "cmp er1, er7\n"
        "bne 1f\n"

        // four at a time
        SET_TEST(3)
        "mov #0x00112233, er2\n"
        "push er2\n"
        "mov #0x44556677, er2\n"
        "push er2\n"
        "mov #0x8899AABB, er2\n"
        "push er2\n"
        "mov #0xCCDDEEFF, er2\n"
        "push er2\n"

        "ldc #0x8F, ccr\n"
        "ldm @er7+, er0-er3\n"
        CHECK_COND_NZVC
        CHECK_EQUAL(0, 0x00112233)
        CHECK_EQUAL(1, 0x44556677)
        CHECK_EQUAL(2, 0x8899AABB)
        CHECK_EQUAL(3, 0xCCDDEEFF)

        // make sure stack is where we started
        "mov %[mem], er1\n"
        "cmp er1, er7\n"
        "bne 1f\n"

        // success
        "mov #0, r0\n"
        "mov r0, %[res]\n"

        "1:\n"
        "mov er7, er3\n"
        "mov %[mem], er7\n" // restore sp
        COPY_REGS
        : [res] "+m" (res), REGS_VARS
        : [mem] "m" (testData)
        : "er0", "er1", "er2", "er3", "er4", "er5", "er6", "cc"
    );

    #undef TEST32
    #undef TEST16
    #undef TEST8

    if(res == 0)
        writeString(".\n");
    else
    {
        char buf [50];
        snprintf(buf, sizeof(buf), " failed at %i. ", res);
        writeString(buf);
        dumpRegs(regs);
        writeString("\n");
    }
}

void stmTest()
{
    writeString("STM");

    int res = 1;
    long regs[8];
    //char cc;


    asm volatile(
        "mov er7, %[mem]\n" // save stack in mem (we're about to use every register)

        // stm
        "mov #0x00112233, er0\n"
        "mov #0x44556677, er1\n"
        "ldc #0x80, ccr\n"
        "stm er0-er1, @-er7\n"
        CHECK_COND_nzvc
        "mov #0x8899AABB, er2\n"
        "mov #0xCCDDEEFF, er3\n"
        "ldc #0x8F, ccr\n"
        "stm er2-er3, @-er7\n"
        CHECK_COND_NZVC
        "mov #0xFFEEDDCC, er4\n"
        "mov #0xBBAA9988, er5\n"
        "stm er4-er5, @-er7\n"

        // check values
        "pop er0\n"
        CHECK_EQUAL(0, 0xBBAA9988)
        "pop er0\n"
        CHECK_EQUAL(0, 0xFFEEDDCC)
        "pop er0\n"
        CHECK_EQUAL(0, 0xCCDDEEFF)
        "pop er0\n"
        CHECK_EQUAL(0, 0x8899AABB)
        "pop er0\n"
        CHECK_EQUAL(0, 0x44556677)
        "pop er0\n"
        CHECK_EQUAL(0, 0x00112233)

        // make sure stack is where we started
        "mov %[mem], er1\n"
        "cmp er1, er7\n"
        "bne 1f\n"

        // three at a time
        SET_TEST(2)
        "mov #0x00112233, er0\n"
        "mov #0x44556677, er1\n"
        "mov #0x8899AABB, er2\n"
        "ldc #0x80, ccr\n"
        "stm er0-er2, @-er7\n"
        CHECK_COND_nzvc
        "mov #0xCCDDEEFF, er4\n"
        "mov #0xFFEEDDCC, er5\n"
        "mov #0xBBAA9988, er6\n"
        "ldc #0x8F, ccr\n"
        "stm er4-er6, @-er7\n"
        CHECK_COND_NZVC

        // check values
        "pop er0\n"
        CHECK_EQUAL(0, 0xBBAA9988)
        "pop er0\n"
        CHECK_EQUAL(0, 0xFFEEDDCC)
        "pop er0\n"
        CHECK_EQUAL(0, 0xCCDDEEFF)
        "pop er0\n"
        CHECK_EQUAL(0, 0x8899AABB)
        "pop er0\n"
        CHECK_EQUAL(0, 0x44556677)
        "pop er0\n"
        CHECK_EQUAL(0, 0x00112233)

        // make sure stack is where we started
        "mov %[mem], er1\n"
        "cmp er1, er7\n"
        "bne 1f\n"

        // four at a time
        SET_TEST(3)
        "mov #0x00112233, er0\n"
        "mov #0x44556677, er1\n"
        "mov #0x8899AABB, er2\n"
        "mov #0xCCDDEEFF, er3\n"
        "ldc #0x8F, ccr\n"
        "stm er0-er3, @-er7\n"
        CHECK_COND_NZVC

        // check values
        "pop er0\n"
        CHECK_EQUAL(0, 0xCCDDEEFF)
        "pop er0\n"
        CHECK_EQUAL(0, 0x8899AABB)
        "pop er0\n"
        CHECK_EQUAL(0, 0x44556677)
        "pop er0\n"
        CHECK_EQUAL(0, 0x00112233)

        // make sure stack is where we started
        "mov %[mem], er1\n"
        "cmp er1, er7\n"
        "bne 1f\n"

        // success
        "mov #0, r0\n"
        "mov r0, %[res]\n"

        "1:\n"
        "mov er7, er3\n"
        "mov %[mem], er7\n" // restore sp
        COPY_REGS
        : [res] "+m" (res), REGS_VARS
        : [mem] "m" (testData)
        : "er0", "er1", "er2", "er3", "er4", "er5", "er6", "cc"
    );

    #undef TEST32
    #undef TEST16
    #undef TEST8

    if(res == 0)
        writeString(".\n");
    else
    {
        char buf [50];
        snprintf(buf, sizeof(buf), " failed at %i. ", res);
        writeString(buf);
        dumpRegs(regs);
        writeString("\n");
    }
}
