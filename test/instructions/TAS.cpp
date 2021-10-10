#include <stdio.h>

#include "Registers.hpp"

#include "helpers.h"

void tasTest()
{
    writeString("TAS");

    int res = 1;
    long regs[8];
    unsigned char cc;

    // only valid on er0,1,4,5
    asm volatile(
        "mov %[mem], er1\n"
        // 0
        "mov #0, r0l\n"
        "mov r0l, @er1\n"
        "ldc #0x80, ccr\n"
        "tas @er1\n"
        CHECK_COND_nZvc
        "mov @er1, r0l\n"
        CHECK_EQUAL(0, 0x80)

        // now 0x80
        SET_TEST(2)
        "tas @er1\n"
        CHECK_COND_Nzvc
        "mov @er1, r0l\n"
        CHECK_EQUAL(0, 0x80)

        SET_TEST(3)
        "mov #1, r0l\n"
        "mov r0l, @er1\n"
        "tas @er1\n"
        CHECK_COND_nzvc
        "mov @er1, r0l\n"
        CHECK_EQUAL(0, 0x81)

        // success
        "mov #0, r0\n"
        "mov r0, %[res]\n"
        "1:\n"
        "stc ccr, %[cc]l\n"
        COPY_REGS
        : [res] "+m" (res), REGS_VARS, [cc] "=r" (cc)
        : [mem] "i" (&testData[0])
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
