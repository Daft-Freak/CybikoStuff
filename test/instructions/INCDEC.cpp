#include <stdio.h>

#include "Registers.hpp"

#include "helpers.h"

void incTest()
{
    writeString("INC");

    int res = 1;
    long regs[8];
    unsigned char cc;

    asm volatile(
        // .b + 1
        "mov #0, er1\n"
        "inc r1l\n"
        CHECK_COND_nzvc
        CHECK_EQUAL(1, 0x01)

        SET_TEST(2)
        "mov #0x7F, r1l\n"
        "inc r1l\n"
        CHECK_COND_NzVc
        CHECK_EQUAL(1, 0x80)

        SET_TEST(3)
        "mov #0x80, r1l\n"
        "inc r1l\n"
        CHECK_COND_Nzvc
        CHECK_EQUAL(1, 0x81)

        SET_TEST(4)
        "mov #0xFF, r1l\n"
        "inc r1l\n"
        CHECK_COND_nZvc
        CHECK_EQUAL(1, 0x00)

        // .w + 1
        SET_TEST(5)
        "mov #0, er1\n"
        "inc #1, r1\n"
        CHECK_COND_nzvc
        CHECK_EQUAL(1, 0x0001)

        SET_TEST(6)
        "mov #0x7FFF, r1\n"
        "inc #1, r1\n"
        CHECK_COND_NzVc
        CHECK_EQUAL(1, 0x8000)

        SET_TEST(7)
        "mov #0x8000, r1\n"
        "inc #1, r1\n"
        CHECK_COND_Nzvc
        CHECK_EQUAL(1, 0x8001)

        SET_TEST(8)
        "mov #0xFFFF, r1\n"
        "inc #1, r1\n"
        CHECK_COND_nZvc
        CHECK_EQUAL(1, 0x0000)

        // .w + 2
        SET_TEST(9)
        "mov #0, er1\n"
        "inc #2, r1\n"
        CHECK_COND_nzvc
        CHECK_EQUAL(1, 0x0002)

        SET_TEST(10)
        "mov #0x7FFF, r1\n"
        "inc #2, r1\n"
        CHECK_COND_NzVc
        CHECK_EQUAL(1, 0x8001)

        SET_TEST(11)
        "mov #0x7FFE, r1\n"
        "inc #2, r1\n"
        CHECK_COND_NzVc
        CHECK_EQUAL(1, 0x8000)

        SET_TEST(12)
        "mov #0x8000, r1\n"
        "inc #2, r1\n"
        CHECK_COND_Nzvc
        CHECK_EQUAL(1, 0x8002)

        SET_TEST(13)
        "mov #0xFFFE, r1\n"
        "inc #2, r1\n"
        CHECK_COND_nZvc
        CHECK_EQUAL(1, 0x0000)

        // .l + 1
        SET_TEST(14)
        "mov #0, er1\n"
        "inc #1, er1\n"
        CHECK_COND_nzvc
        CHECK_EQUAL(1, 0x00000001)

        SET_TEST(15)
        "mov #0x7FFFFFFF, er1\n"
        "inc #1, er1\n"
        CHECK_COND_NzVc
        CHECK_EQUAL(1, 0x80000000)

        SET_TEST(16)
        "mov #0x80000000, er1\n"
        "inc #1, er1\n"
        CHECK_COND_Nzvc
        CHECK_EQUAL(1, 0x80000001)

        SET_TEST(17)
        "mov #0xFFFFFFFF, er1\n"
        "inc #1, er1\n"
        CHECK_COND_nZvc
        CHECK_EQUAL(1, 0x00000000)

        // .l + 2
        SET_TEST(18)
        "mov #0, er1\n"
        "inc #2, er1\n"
        CHECK_COND_nzvc
        CHECK_EQUAL(1, 0x00000002)

        SET_TEST(19)
        "mov #0x7FFFFFFF, er1\n"
        "inc #2, er1\n"
        CHECK_COND_NzVc
        CHECK_EQUAL(1, 0x80000001)

        SET_TEST(20)
        "mov #0x7FFFFFFE, er1\n"
        "inc #2, er1\n"
        CHECK_COND_NzVc
        CHECK_EQUAL(1, 0x80000000)

        SET_TEST(21)
        "mov #0x80000000, er1\n"
        "inc #2, er1\n"
        CHECK_COND_Nzvc
        CHECK_EQUAL(1, 0x80000002)

        SET_TEST(22)
        "mov #0xFFFFFFFE, er1\n"
        "inc #2, er1\n"
        CHECK_COND_nZvc
        CHECK_EQUAL(1, 0x00000000)

        // success
        "mov #0, r0\n"
        "mov r0, %[res]\n"
        "1:\n"
        "stc ccr, %[cc]l\n"
        COPY_REGS // not as useful here
        : [res] "+m" (res), REGS_VARS, [cc] "=r" (cc)
        :
        : "er0", "er1", "cc"
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

void decTest()
{
    writeString("DEC");

    int res = 1;
    long regs[8];
    unsigned char cc;

    asm volatile(
        // .b + 1
        "mov #2, er1\n"
        "dec r1l\n"
        CHECK_COND_nzvc
        CHECK_EQUAL(1, 0x01)

        SET_TEST(2)
        "mov #0x80, r1l\n"
        "dec r1l\n"
        CHECK_COND_nzVc
        CHECK_EQUAL(1, 0x7F)

        SET_TEST(3)
        "mov #0x81, r1l\n"
        "dec r1l\n"
        CHECK_COND_Nzvc
        CHECK_EQUAL(1, 0x80)

        SET_TEST(4)
        "mov #0x1, r1l\n"
        "dec r1l\n"
        CHECK_COND_nZvc
        CHECK_EQUAL(1, 0x00)

        // .w + 1
        SET_TEST(5)
        "mov #2, er1\n"
        "dec #1, r1\n"
        CHECK_COND_nzvc
        CHECK_EQUAL(1, 0x0001)

        SET_TEST(6)
        "mov #0x8000, r1\n"
        "dec #1, r1\n"
        CHECK_COND_nzVc
        CHECK_EQUAL(1, 0x7FFF)

        SET_TEST(7)
        "mov #0x8001, r1\n"
        "dec #1, r1\n"
        CHECK_COND_Nzvc
        CHECK_EQUAL(1, 0x8000)

        SET_TEST(8)
        "mov #0x1, r1\n"
        "dec #1, r1\n"
        CHECK_COND_nZvc
        CHECK_EQUAL(1, 0x0000)

        // .w + 2
        SET_TEST(9)
        "mov #3, er1\n"
        "dec #2, r1\n"
        CHECK_COND_nzvc
        CHECK_EQUAL(1, 0x0001)

        SET_TEST(10)
        "mov #0x8000, r1\n"
        "dec #2, r1\n"
        CHECK_COND_nzVc
        CHECK_EQUAL(1, 0x7FFE)

        SET_TEST(11)
        "mov #0x8001, r1\n"
        "dec #2, r1\n"
        CHECK_COND_nzVc
        CHECK_EQUAL(1, 0x7FFF)

        SET_TEST(12)
        "mov #0x8002, r1\n"
        "dec #2, r1\n"
        CHECK_COND_Nzvc
        CHECK_EQUAL(1, 0x8000)

        SET_TEST(13)
        "mov #0x2, r1\n"
        "dec #2, r1\n"
        CHECK_COND_nZvc
        CHECK_EQUAL(1, 0x0000)

        // .l + 1
        SET_TEST(14)
        "mov #2, er1\n"
        "dec #1, er1\n"
        CHECK_COND_nzvc
        CHECK_EQUAL(1, 0x00000001)

        SET_TEST(15)
        "mov #0x80000000, er1\n"
        "dec #1, er1\n"
        CHECK_COND_nzVc
        CHECK_EQUAL(1, 0x7FFFFFFF)

        SET_TEST(16)
        "mov #0x80000001, er1\n"
        "dec #1, er1\n"
        CHECK_COND_Nzvc
        CHECK_EQUAL(1, 0x80000000)

        SET_TEST(17)
        "mov #0x1, er1\n"
        "dec #1, er1\n"
        CHECK_COND_nZvc
        CHECK_EQUAL(1, 0x00000000)

        // .l + 2
        SET_TEST(18)
        "mov #3, er1\n"
        "dec #2, er1\n"
        CHECK_COND_nzvc
        CHECK_EQUAL(1, 0x00000001)

        SET_TEST(19)
        "mov #0x80000001, er1\n"
        "dec #2, er1\n"
        CHECK_COND_nzVc
        CHECK_EQUAL(1, 0x7FFFFFFF)

        SET_TEST(20)
        "mov #0x80000000, er1\n"
        "dec #2, er1\n"
        CHECK_COND_nzVc
        CHECK_EQUAL(1, 0x7FFFFFFE)

        SET_TEST(21)
        "mov #0x80000002, er1\n"
        "dec #2, er1\n"
        CHECK_COND_Nzvc
        CHECK_EQUAL(1, 0x80000000)

        SET_TEST(22)
        "mov #0x2, er1\n"
        "dec #2, er1\n"
        CHECK_COND_nZvc
        CHECK_EQUAL(1, 0x00000000)

        // success
        "mov #0, r0\n"
        "mov r0, %[res]\n"
        "1:\n"
        "stc ccr, %[cc]l\n"
        COPY_REGS // not as useful here
        : [res] "+m" (res), REGS_VARS, [cc] "=r" (cc)
        :
        : "er0", "er1", "cc"
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
