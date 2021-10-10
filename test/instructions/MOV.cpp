#include <stdio.h>

#include "Registers.hpp"

#include "helpers.h"

void movImmTest()
{
    writeString("MOV IMM");

    // also uses some indirect disp for result...
    // (and cmp/bcc)
    // does not test er7 (sp)

    int res = 1;
    long regs[8];
    //char cc;

    #define TEST32(reg) \
        "ldc #0x80, ccr\n" \
        "mov #0, er"#reg"\n" \
        CHECK_COND_nZvc \
        CHECK_EQUAL(reg, 0) \
        "ldc #0x83, ccr\n" \
        "mov #0x11223344, er"#reg"\n" \
        CHECK_COND_nzvC \
        CHECK_EQUAL(reg, 0x11223344) \
        "mov #0x88776655, er"#reg"\n" \
        CHECK_COND_Nzvc \
        CHECK_EQUAL(reg, 0x88776655)

    // En and Rn
    #define TEST16(reg) \
        "ldc #0x80, ccr\n" \
        "mov #0, r"#reg"\n" \
        CHECK_COND_nZvc \
        CHECK_EQUAL(reg, 0x88770000) \
        "ldc #0x83, ccr\n" \
        "mov #0x2211, r"#reg"\n" \
        CHECK_COND_nzvC \
        CHECK_EQUAL(reg, 0x88772211) \
        "mov #0x99AA, r"#reg"\n" \
        CHECK_COND_Nzvc \
        CHECK_EQUAL(reg, 0x887799AA) \
        \
        "ldc #0x80, ccr\n" \
        "mov #0, e"#reg"\n" \
        CHECK_COND_nZvc \
        CHECK_EQUAL(reg, 0x000099aa) \
        "ldc #0x83, ccr\n" \
        "mov #0x2211, e"#reg"\n" \
        CHECK_COND_nzvC \
        CHECK_EQUAL(reg, 0x221199AA) \
        "mov #0xBBCC, e"#reg"\n" \
        CHECK_COND_Nzvc \
        CHECK_EQUAL(reg, 0xBBCC99AA)

    // RnH and RnL
    #define TEST8(reg) \
        "ldc #0x80, ccr\n" \
        "mov #0, r"#reg"l\n" \
        CHECK_COND_nZvc \
        CHECK_EQUAL(reg, 0xBBCC9900) \
        "ldc #0x83, ccr\n" \
        "mov #0x11, r"#reg"l\n" \
        CHECK_COND_nzvC \
        CHECK_EQUAL(reg, 0xBBCC9911) \
        "mov #0x88, r"#reg"l\n" \
        CHECK_COND_Nzvc \
        CHECK_EQUAL(reg, 0xBBCC9988) \
        \
        "ldc #0x80, ccr\n" \
        "mov #0, r"#reg"h\n" \
        CHECK_COND_nZvc \
        CHECK_EQUAL(reg, 0xBBCC0088) \
        "ldc #0x83, ccr\n" \
        "mov #0x11, r"#reg"h\n" \
        CHECK_COND_nzvC \
        CHECK_EQUAL(reg, 0xBBCC1188) \
        "mov #0x99, r"#reg"h\n" \
        CHECK_COND_Nzvc \
        CHECK_EQUAL(reg, 0xBBCC9988)


    asm volatile(
        // test 1-7: er0-6
        TEST32(0)
        SET_TEST(2) TEST32(1)
        SET_TEST(3) TEST32(2)
        SET_TEST(4) TEST32(3)
        SET_TEST(5) TEST32(4)
        SET_TEST(6) TEST32(5)
        SET_TEST(7) TEST32(6)


        // test 8-14: r0-6/e0-6
        SET_TEST(8)
        // reset er0
        "mov #0x88776655, er0\n"
        TEST16(0)
        SET_TEST(9) TEST16(1)
        SET_TEST(10) TEST16(2)
        SET_TEST(11) TEST16(3)
        SET_TEST(12) TEST16(4)
        SET_TEST(13) TEST16(5)
        SET_TEST(14) TEST16(6)

        // test 15-21: r0-6l/r0-6l
        SET_TEST(15)
        // reset er0
        "mov #0xBBCC99AA, er0\n"
        TEST8(0)
        SET_TEST(16) TEST8(1)
        SET_TEST(17) TEST8(2)
        SET_TEST(18) TEST8(3)
        SET_TEST(19) TEST8(4)
        SET_TEST(20) TEST8(5)
        SET_TEST(21) TEST8(6)

        // success
        "mov #0, r0\n"
        "mov r0, %[res]\n"
        "1:\n"
        COPY_REGS
        : [res] "+m" (res), REGS_VARS
        :
        : "er0", "er1", "er2", "er3", "er4", "er5", "er6", "cc"
    );

    #undef TEST32
    #undef TEST16
    #undef TEST8

    if(res == 0)
        writeString(".\n");
    else
    {
        char buf[50];
        snprintf(buf, sizeof(buf), " failed at %i. ", res);
        writeString(buf);
        dumpRegs(regs);
        writeString("\n");
    }
}

void movRegTest()
{
    writeString("MOV REG");

    int res = 1;
    long regs[8];
    //char cc;

    // like the imm test, but goes through a register
    #define TEST32(reg) \
        "mov #0x00000000, er0\n" \
        "ldc #0x80, ccr\n" \
        "mov er0, er"#reg"\n" \
        CHECK_COND_nZvc \
        CHECK_EQUAL(reg, 0) \
        \
        "mov #0x11223344, er1\n" \
        "ldc #0x83, ccr\n" \
        "mov er1, er"#reg"\n" \
        CHECK_COND_nzvC \
        "cmp #0x11223344, er"#reg"\n" \
        \
        "mov #0x55667788, er2\n" \
        "ldc #0x8F, ccr\n" \
        "mov er2, er"#reg"\n" \
        CHECK_COND_nzvC \
        CHECK_EQUAL(reg, 0x55667788) \
        \
        "mov #0x99AABBCC, er3\n" \
        "ldc #0x87, ccr\n" \
        "mov er3, er"#reg"\n" \
        CHECK_COND_NzvC \
        CHECK_EQUAL(reg, 0x99AABBCC) \
        \
        "mov #0xDDEEFFEE, er4\n" \
        "ldc #0x80, ccr\n" \
        "mov er4, er"#reg"\n" \
        CHECK_COND_Nzvc \
        CHECK_EQUAL(reg, 0xDDEEFFEE) \
        \
        "mov #0xDDCCBBAA, er5\n" \
        "ldc #0x83, ccr\n" \
        "mov er5, er"#reg"\n" \
        CHECK_COND_NzvC \
        CHECK_EQUAL(reg, 0xDDCCBBAA) \
        \
        "mov #0x99887766, er6\n" \
        "ldc #0x87, ccr\n" \
        "mov er6, er"#reg"\n" \
        CHECK_COND_NzvC \
        CHECK_EQUAL(reg, 0x99887766)

    // uses 8/16-bit mov to do a 64-bit bswap
    // should maybe check some flags...
    #define TEST_SWAP(regA, regB, regT) \
        "mov #0x00112233, er"#regA"\n" \
        "mov #0x44556677, er"#regB"\n" \
        \
        "mov r"#regB"h, r"#regT"l\n" \
        "mov r"#regB"l, r"#regB"h\n" \
        "mov r"#regT"l, r"#regB"l\n" \
        \
        "mov e"#regA", r"#regT"\n" \
        "mov r"#regB", e"#regA"\n" \
        "mov r"#regT", r"#regB"\n" \
        \
        "mov r"#regB"h, r"#regT"l\n" \
        "mov r"#regB"l, r"#regB"h\n" \
        "mov r"#regT"l, r"#regB"l\n" \
        \
        CHECK_EQUAL(regA, 0x77662233) \
        CHECK_EQUAL(regB, 0x44551100) \
        \
        "mov r"#regA"h, r"#regT"l\n" \
        "mov r"#regA"l, r"#regA"h\n" \
        "mov r"#regT"l, r"#regA"l\n" \
        \
        "mov e"#regB", r"#regT"\n" \
        "mov r"#regA", e"#regB"\n" \
        "mov r"#regT", r"#regA"\n" \
        \
        "mov r"#regA"h, r"#regT"l\n" \
        "mov r"#regA"l, r"#regA"h\n" \
        "mov r"#regT"l, r"#regA"l\n" \
        \
        CHECK_EQUAL(regA, 0x77665544) \
        CHECK_EQUAL(regB, 0x33221100)

    asm volatile(
        //test 1-7: er0-6
        TEST32(0)
        SET_TEST(2) TEST32(1)
        SET_TEST(3) TEST32(2)
        SET_TEST(4) TEST32(3)
        SET_TEST(5) TEST32(4)
        SET_TEST(6) TEST32(5)
        SET_TEST(7) TEST32(6)

        //test 8-11: 8/16-bit
        SET_TEST(8) TEST_SWAP(0, 1, 2)
        SET_TEST(9) TEST_SWAP(2, 3, 4)
        SET_TEST(10) TEST_SWAP(4, 5, 6)
        SET_TEST(11) TEST_SWAP(6, 0, 1)

        "mov #0, r0\n"
        "mov r0, %[res]\n"
        "1:\n"
        COPY_REGS
        : [res] "+m" (res), REGS_VARS
        :
        : "er0", "er1", "er2", "er3", "er4", "er5", "er6", "cc"
    );

    #undef TEST32
    #undef TEST_SWAP

    if(res == 0)
        writeString(".\n");
    else
    {
        char buf[50];
        snprintf(buf, sizeof(buf), " failed at %i. ", res);
        writeString(buf);
        dumpRegs(regs);
        writeString("\n");
    }
}

void movIndirectTest()
{
    writeString("MOV INDIRECT");

    int res = 1;
    long regs[8];
    //char cc;

    // like the immediate test, but goes through memory

    #define MOV_AND_TEST_COND(reg, aReg, cond) \
        "mov "#reg", @er"#aReg"\n"\
        cond \
        "mov @er"#aReg", "#reg"\n"\
        cond

    #define TEST32(reg, aReg) \
        "mov %[mem], er"#aReg"\n" \
        \
        "mov #0, er"#reg"\n" \
        "ldc #0x80, ccr\n" \
        MOV_AND_TEST_COND(er##reg, aReg, CHECK_COND_nZvc) \
        CHECK_EQUAL(reg, 0) \
        \
        "mov #0x11223344, er"#reg"\n" \
        "ldc #0x83, ccr\n" \
        MOV_AND_TEST_COND(er##reg, aReg, CHECK_COND_nzvC) \
        CHECK_EQUAL(reg, 0x11223344) \
        \
        "mov #0x88776655, er"#reg"\n" \
        MOV_AND_TEST_COND(er##reg, aReg, CHECK_COND_Nzvc) \
        CHECK_EQUAL(reg, 0x88776655)

    // En and Rn
    #define TEST16(reg, aReg) \
        "mov #0x88776655, er"#reg"\n" \
        "mov %[mem], er"#aReg"\n" \
        \
        "mov #0, r"#reg"\n" \
        "ldc #0x80, ccr\n" \
        MOV_AND_TEST_COND(r##reg, aReg, CHECK_COND_nZvc) \
        CHECK_EQUAL(reg, 0x88770000) \
        \
        "mov #0x2211, r"#reg"\n" \
        "ldc #0x83, ccr\n" \
        MOV_AND_TEST_COND(r##reg, aReg, CHECK_COND_nzvC) \
        CHECK_EQUAL(reg, 0x88772211) \
        \
        "mov #0x99AA, r"#reg"\n" \
        MOV_AND_TEST_COND(r##reg, aReg, CHECK_COND_Nzvc) \
        CHECK_EQUAL(reg, 0x887799AA) \
        \
        \
        "mov #0, e"#reg"\n" \
        "ldc #0x80, ccr\n" \
        MOV_AND_TEST_COND(e##reg, aReg, CHECK_COND_nZvc) \
        CHECK_EQUAL(reg, 0x000099aa) \
        \
        "mov #0x2211, e"#reg"\n" \
        "ldc #0x83, ccr\n" \
        MOV_AND_TEST_COND(e##reg, aReg, CHECK_COND_nzvC) \
        CHECK_EQUAL(reg, 0x221199AA) \
        \
        "mov #0xBBCC, e"#reg"\n" \
        MOV_AND_TEST_COND(e##reg, aReg, CHECK_COND_Nzvc) \
        CHECK_EQUAL(reg, 0xBBCC99AA)

    // RnH and RnL
    #define TEST8(reg, aReg) \
        "mov #0xBBCC99AA, er"#reg"\n" \
        "mov %[mem], er"#aReg"\n" \
        \
        "mov #0, r"#reg"l\n" \
        "ldc #0x80, ccr\n" \
        MOV_AND_TEST_COND(r##reg##l, aReg, CHECK_COND_nZvc) \
        CHECK_EQUAL(reg, 0xBBCC9900) \
        \
        "mov #0x11, r"#reg"l\n" \
        "ldc #0x83, ccr\n" \
        MOV_AND_TEST_COND(r##reg##l, aReg, CHECK_COND_nzvC) \
        CHECK_EQUAL(reg, 0xBBCC9911) \
        \
        "mov #0x88, r"#reg"l\n" \
        MOV_AND_TEST_COND(r##reg##l, aReg, CHECK_COND_Nzvc) \
        CHECK_EQUAL(reg, 0xBBCC9988) \
        \
        \
        "mov #0, r"#reg"h\n" \
        "ldc #0x80, ccr\n" \
        MOV_AND_TEST_COND(r##reg##h, aReg, CHECK_COND_nZvc) \
        CHECK_EQUAL(reg, 0xBBCC0088) \
        \
        "mov #0x11, r"#reg"h\n" \
        "ldc #0x83, ccr\n" \
        MOV_AND_TEST_COND(r##reg##h, aReg, CHECK_COND_nzvC) \
        CHECK_EQUAL(reg, 0xBBCC1188) \
        \
        "mov #0x99, r"#reg"h\n" \
        MOV_AND_TEST_COND(r##reg##h, aReg, CHECK_COND_Nzvc) \
        CHECK_EQUAL(reg, 0xBBCC9988)


    asm volatile(
        //test 1-7: er0-6
        TEST32(0, 1)
        SET_TEST(2) TEST32(1, 2)
        SET_TEST(3) TEST32(2, 3)
        SET_TEST(4) TEST32(3, 4)
        SET_TEST(5) TEST32(4, 5)
        SET_TEST(6) TEST32(5, 6)
        SET_TEST(7) TEST32(6, 0)


        //test 8-14: r0-6/e0-6
        SET_TEST(8) TEST16(0, 1)
        SET_TEST(9) TEST16(1, 2)
        SET_TEST(10) TEST16(2, 3)
        SET_TEST(11) TEST16(3, 4)
        SET_TEST(12) TEST16(4, 5)
        SET_TEST(13) TEST16(5, 6)
        SET_TEST(14) TEST16(6, 0)

        //test 15: r0-6l/r0-6h
        SET_TEST(15) TEST8(0, 1)
        SET_TEST(16) TEST8(1, 2)
        SET_TEST(17) TEST8(2, 3)
        SET_TEST(18) TEST8(3, 4)
        SET_TEST(19) TEST8(4, 5)
        SET_TEST(20) TEST8(5, 6)
        SET_TEST(21) TEST8(6, 0)

        "mov #0, r0\n"
        "mov r0, %[res]\n"
        "1:\n"
        COPY_REGS
        : [res] "+m" (res), REGS_VARS
        : [mem] "i" (&testData[0])
        : "er0", "er1", "er2", "er3", "er4", "er5", "er6", "cc"
    );

    #undef TEST32
    #undef TEST16
    #undef TEST8
    #undef MOV_AND_TEST_COND

    if(res == 0)
        writeString(".\n");
    else
    {
        char buf[50];
        snprintf(buf, sizeof(buf), " failed at %i. ", res);
        writeString(buf);
        dumpRegs(regs);
        writeString("\n");
    }
}

void movIndirectDispTest()
{
    writeString("MOV INDIRECT DISP (READ)");

    int res = 1;
    long regs[8];
    //char cc;

    // reads through a displacement

    // not checking the condition flags on the non-displaced one as we just tested that
    #define MOV_AND_TEST_COND(reg, aReg, addr, disp, cond) \
        "mov %[mem1], er"#aReg"\n" \
        "mov "#reg", @er"#aReg"\n"\
        "mov %["#addr"], er"#aReg"\n" \
        "mov @("#disp", er"#aReg"), "#reg"\n"\
        cond

    #define TEST32(reg, aReg) \
        "mov %[mem1], er"#aReg"\n" \
        \
        "mov #0, er"#reg"\n" \
        "ldc #0x80, ccr\n" \
        MOV_AND_TEST_COND(er##reg, aReg, mem0, 4:16, CHECK_COND_nZvc) \
        CHECK_EQUAL(reg, 0) \
        \
        "mov #0x11223344, er"#reg"\n" \
        "ldc #0x83, ccr\n" \
        MOV_AND_TEST_COND(er##reg, aReg, mem2, -4:16, CHECK_COND_nzvC) \
        CHECK_EQUAL(reg, 0x11223344) \
        \
        "mov #0x88776655, er"#reg"\n" \
        MOV_AND_TEST_COND(er##reg, aReg, mem0, 4:32, CHECK_COND_Nzvc) \
        CHECK_EQUAL(reg, 0x88776655) \
        \
        "mov #0x99AABBCC, er"#reg"\n" \
        MOV_AND_TEST_COND(er##reg, aReg, mem2, -4:32, CHECK_COND_Nzvc) \
        CHECK_EQUAL(reg, 0x99AABBCC)

    // En and Rn
    #define TEST16(reg, aReg) \
        "mov #0x88776655, er"#reg"\n" \
        "mov %[mem0], er"#aReg"\n" \
        \
        "mov #0, r"#reg"\n" \
        "ldc #0x80, ccr\n" \
        MOV_AND_TEST_COND(r##reg, aReg, mem0, 4:16, CHECK_COND_nZvc) \
        CHECK_EQUAL(reg, 0x88770000) \
        \
        "mov #0x2211, r"#reg"\n" \
        "ldc #0x83, ccr\n" \
        MOV_AND_TEST_COND(r##reg, aReg, mem2, -4:16, CHECK_COND_nzvC) \
        CHECK_EQUAL(reg, 0x88772211) \
        \
        "mov #0x99AA, r"#reg"\n" \
        MOV_AND_TEST_COND(r##reg, aReg, mem0, 4:32, CHECK_COND_Nzvc) \
        CHECK_EQUAL(reg, 0x887799AA) \
        \
        "mov #0xCCBB, r"#reg"\n" \
        MOV_AND_TEST_COND(r##reg, aReg, mem2, -4:32, CHECK_COND_Nzvc) \
        CHECK_EQUAL(reg, 0x8877CCBB) \
        \
        \
        "mov #0, e"#reg"\n" \
        "ldc #0x80, ccr\n" \
        MOV_AND_TEST_COND(e##reg, aReg, mem0, 4:16, CHECK_COND_nZvc) \
        CHECK_EQUAL(reg, 0x0000CCBB) \
        \
        "mov #0x2211, e"#reg"\n" \
        "ldc #0x83, ccr\n" \
        MOV_AND_TEST_COND(e##reg, aReg, mem2, -4:16, CHECK_COND_nzvC) \
        CHECK_EQUAL(reg, 0x2211CCBB) \
        \
        "mov #0xBBCC, e"#reg"\n" \
        MOV_AND_TEST_COND(e##reg, aReg, mem0, 4:32, CHECK_COND_Nzvc) \
        CHECK_EQUAL(reg, 0xBBCCCCBB) \
        \
        "mov #0xAA99, e"#reg"\n" \
        MOV_AND_TEST_COND(e##reg, aReg, mem2, -4:32, CHECK_COND_Nzvc) \
        CHECK_EQUAL(reg, 0xAA99CCBB)

    // RnH and RnL
    #define TEST8(reg, aReg) \
        "mov #0xBBCC99AA, er"#reg"\n" \
        "mov %[mem0], er"#aReg"\n" \
        \
        "mov #0, r"#reg"l\n" \
        "ldc #0x80, ccr\n" \
        MOV_AND_TEST_COND(r##reg##l, aReg, mem0, 4:16, CHECK_COND_nZvc) \
        "bne 1f:16\n" \
        \
        "mov #0x11, r"#reg"l\n" \
        "ldc #0x83, ccr\n" \
        MOV_AND_TEST_COND(r##reg##l, aReg, mem2, -4:16, CHECK_COND_nzvC) \
        CHECK_EQUAL(reg, 0xBBCC9911) \
        \
        "mov #0x88, r"#reg"l\n" \
        MOV_AND_TEST_COND(r##reg##l, aReg, mem0, 4:32, CHECK_COND_Nzvc) \
        CHECK_EQUAL(reg, 0xBBCC9988) \
        \
        "mov #0xDD, r"#reg"l\n" \
        MOV_AND_TEST_COND(r##reg##l, aReg, mem2, -4:32, CHECK_COND_Nzvc) \
        CHECK_EQUAL(reg, 0xBBCC99DD) \
        \
        \
        "mov #0, r"#reg"h\n" \
        "ldc #0x80, ccr\n" \
        MOV_AND_TEST_COND(r##reg##h, aReg, mem0, 4:16, CHECK_COND_nZvc) \
        CHECK_EQUAL(reg, 0xBBCC00DD) \
        \
        "mov #0x11, r"#reg"h\n" \
        "ldc #0x83, ccr\n" \
        MOV_AND_TEST_COND(r##reg##h, aReg, mem2, -4:16, CHECK_COND_nzvC) \
        CHECK_EQUAL(reg, 0xBBCC11DD) \
        \
        "mov #0x99, r"#reg"h\n" \
        MOV_AND_TEST_COND(r##reg##h, aReg, mem0, 4:32, CHECK_COND_Nzvc) \
        CHECK_EQUAL(reg, 0xBBCC99DD) \
        \
        "mov #0xEE, r"#reg"h\n" \
        MOV_AND_TEST_COND(r##reg##l, aReg, mem2, -4:32, CHECK_COND_Nzvc) \
        CHECK_EQUAL(reg, 0xBBCCEEDD) \


    asm volatile(
        //test 1-7: er0-6
        TEST32(0, 1)
        SET_TEST(2) TEST32(1, 2)
        SET_TEST(3) TEST32(2, 3)
        SET_TEST(4) TEST32(3, 4)
        SET_TEST(5) TEST32(4, 5)
        SET_TEST(6) TEST32(5, 6)
        SET_TEST(7) TEST32(6, 0)


        //test 8-14: r0-6/e0-6
        SET_TEST(8) TEST16(0, 1)
        SET_TEST(9) TEST16(1, 2)
        SET_TEST(10) TEST16(2, 3)
        SET_TEST(11) TEST16(3, 4)
        SET_TEST(12) TEST16(4, 5)
        SET_TEST(13) TEST16(5, 6)
        SET_TEST(14) TEST16(6, 0)

        //test 15: r0-6l/r0-6h
        SET_TEST(15) TEST8(0, 1)
        SET_TEST(16) TEST8(1, 2)
        SET_TEST(17) TEST8(2, 3)
        SET_TEST(18) TEST8(3, 4)
        SET_TEST(19) TEST8(4, 5)
        SET_TEST(20) TEST8(5, 6)
        SET_TEST(21) TEST8(6, 0)

        "mov #0, r0\n"
        "mov r0, %[res]\n"
        "1:\n"
        COPY_REGS
        : [res] "+m" (res), REGS_VARS
        : [mem0] "i" (&testData[0]), [mem1] "i" (&testData[1]), [mem2] "i" (&testData[2])
        : "er0", "er1", "er2", "er3", "er4", "er5", "er6", "cc"
    );

    if(res == 0)
        writeString(".\n");
    else
    {
        char buf[50];
        snprintf(buf, sizeof(buf), " failed at %i. ", res);
        writeString(buf);
        dumpRegs(regs);
        writeString("\n");
    }

    // ... and then for writes
    writeString("MOV INDIRECT DISP (WRITE)");

    #undef MOV_AND_TEST_COND
    #define MOV_AND_TEST_COND(reg, aReg, addr, disp, cond) \
        "mov %["#addr"], er"#aReg"\n" \
        "mov "#reg", @("#disp", er"#aReg")\n" \
        cond \
        "mov %[mem1], er"#aReg"\n" \
        "mov @er"#aReg", "#reg"\n"

    asm volatile(
        //test 1-7: er0-6
        SET_TEST(1) TEST32(0, 1)
        SET_TEST(2) TEST32(1, 2)
        SET_TEST(3) TEST32(2, 3)
        SET_TEST(4) TEST32(3, 4)
        SET_TEST(5) TEST32(4, 5)
        SET_TEST(6) TEST32(5, 6)
        SET_TEST(7) TEST32(6, 0)


        //test 8-14: r0-6/e0-6
        SET_TEST(8) TEST16(0, 1)
        SET_TEST(9) TEST16(1, 2)
        SET_TEST(10) TEST16(2, 3)
        SET_TEST(11) TEST16(3, 4)
        SET_TEST(12) TEST16(4, 5)
        SET_TEST(13) TEST16(5, 6)
        SET_TEST(14) TEST16(6, 0)

        //test 15: r0-6l/r0-6h
        SET_TEST(15) TEST8(0, 1)
        SET_TEST(16) TEST8(1, 2)
        SET_TEST(17) TEST8(2, 3)
        SET_TEST(18) TEST8(3, 4)
        SET_TEST(19) TEST8(4, 5)
        SET_TEST(20) TEST8(5, 6)
        SET_TEST(21) TEST8(6, 0)

        "mov #0, r0\n"
        "mov r0, %[res]\n"
        "1:\n"
        COPY_REGS
        : [res] "+m" (res), REGS_VARS
        : [mem0] "i" (&testData[0]), [mem1] "i" (&testData[1]), [mem2] "i" (&testData[2])
        : "er0", "er1", "er2", "er3", "er4", "er5", "er6", "cc"
    );

    #undef TEST32
    #undef TEST16
    #undef TEST8
    #undef MOV_AND_TEST_COND

    if(res == 0)
        writeString(".\n");
    else
    {
        char buf[50];
        snprintf(buf, sizeof(buf), " failed at %i. ", res);
        writeString(buf);
        dumpRegs(regs);
        writeString("\n");
    }
}

void movIndirectIncTest()
{
    writeString("MOV INDIRECT INC");

    int res = 1;
    long regs[8];
    //char cc;

    // reads through incrementing reg

    #define TEST32(reg, aReg) \
        "mov %[mem0], er"#aReg"\n" \
        \
        "ldc #0x80, ccr\n" \
        "mov @er"#aReg"+, er"#reg"\n" \
        CHECK_COND_nZvc \
        CHECK_EQUAL(reg, 0) \
        \
        "ldc #0x83, ccr\n" \
        "mov @er"#aReg"+, er"#reg"\n" \
        CHECK_COND_nzvC \
        CHECK_EQUAL(reg, 0x11223344) \
        \
        "mov @er"#aReg"+, er"#reg"\n" \
        CHECK_COND_Nzvc \
        CHECK_EQUAL(reg, 0x88776655)

    // En and Rn
    #define TEST16(reg, aReg) \
        "mov #0x88776655, er"#reg"\n" \
        "mov %[mem0], er"#aReg"\n" \
        \
        "ldc #0x80, ccr\n" \
        "mov @er"#aReg"+, r"#reg"\n" \
        CHECK_COND_nZvc \
        CHECK_EQUAL(reg, 0x88770000) \
        "mov @er"#aReg"+, r"#reg"\n"/*skip to next*/\
        \
        "ldc #0x83, ccr\n" \
        "mov @er"#aReg"+, r"#reg"\n" \
        CHECK_COND_nzvC \
        CHECK_EQUAL(reg, 0x88771122) \
        "mov @er"#aReg"+, r"#reg"\n"/*skip to next*/\
        \
        "mov @er"#aReg"+, r"#reg"\n" \
        CHECK_COND_Nzvc \
        CHECK_EQUAL(reg, 0x88778877) \
        \
        \
        "mov %[mem0], er"#aReg"\n" \
        \
        "ldc #0x80, ccr\n" \
        "mov @er"#aReg"+, e"#reg"\n"/*skip to next*/\
        CHECK_COND_nZvc \
        CHECK_EQUAL(reg, 0x00008877) \
        "mov @er"#aReg"+, e"#reg"\n"/*skip to next*/\
        \
        "ldc #0x83, ccr\n" \
        "mov @er"#aReg"+, e"#reg"\n" \
        CHECK_COND_nzvC \
        CHECK_EQUAL(reg, 0x11228877) \
        "mov @er"#aReg"+, e"#reg"\n"/*skip to next*/\
        \
        "mov @er"#aReg"+, e"#reg"\n" \
        CHECK_COND_Nzvc \
        CHECK_EQUAL(reg, 0x88778877)

    // RnH and RnL
    #define SKIP3(reg, aReg) "mov @er"#aReg"+, r"#reg"\n mov @er"#aReg"+, r"#reg"\n mov @er"#aReg"+, r"#reg"\n"

    #define TEST8(reg, aReg) \
        "mov #0xBBCC99AA, er"#reg"\n" \
        "mov %[mem0], er"#aReg"\n" \
        \
        "ldc #0x80, ccr\n" \
        "mov @er"#aReg"+, r"#reg"l\n" \
        CHECK_COND_nZvc \
        CHECK_EQUAL(reg, 0xBBCC9900) \
        SKIP3(reg##l, aReg) \
        \
        "ldc #0x83, ccr\n" \
        "mov @er"#aReg"+, r"#reg"l\n" \
        CHECK_COND_nzvC \
        CHECK_EQUAL(reg, 0xBBCC9911) \
        SKIP3(reg##l, aReg) \
        \
        "mov @er"#aReg"+, r"#reg"l\n" \
        CHECK_COND_Nzvc \
        CHECK_EQUAL(reg, 0xBBCC9988) \
        \
        \
        "mov %[mem0], er"#aReg"\n" \
        \
        "ldc #0x80, ccr\n" \
        "mov @er"#aReg"+, r"#reg"h\n" \
        CHECK_COND_nZvc \
        CHECK_EQUAL(reg, 0xBBCC0088) \
        SKIP3(reg##h, aReg) \
        \
        "ldc #0x83, ccr\n" \
        "mov @er"#aReg"+, r"#reg"h\n" \
        CHECK_COND_nzvC \
        CHECK_EQUAL(reg, 0xBBCC1188) \
        SKIP3(reg##h, aReg) \
        \
        "mov @er"#aReg"+, r"#reg"h\n" \
        CHECK_COND_Nzvc \
        CHECK_EQUAL(reg, 0xBBCC8888)


    asm volatile(
        // init (these address modes have been tested by now)
        "mov %[mem0], er1\n"
        "mov #0, er0\n"
        "mov er0, @er1\n"

        "mov %[mem1], er1\n"
        "mov #0x11223344, er0\n"
        "mov er0, @er1\n"

        "mov %[mem2], er1\n"
        "mov #0x88776655, er0\n"
        "mov er0, @er1\n"

        //test 1-7: er0-6
        TEST32(0, 1)
        SET_TEST(2) TEST32(1, 2)
        SET_TEST(3) TEST32(2, 3)
        SET_TEST(4) TEST32(3, 4)
        SET_TEST(5) TEST32(4, 5)
        SET_TEST(6) TEST32(5, 6)
        SET_TEST(7) TEST32(6, 0)


        //test 8-14: r0-6/e0-6
        SET_TEST(8) TEST16(0, 1)
        SET_TEST(9) TEST16(1, 2)
        SET_TEST(10) TEST16(2, 3)
        SET_TEST(11) TEST16(3, 4)
        SET_TEST(12) TEST16(4, 5)
        SET_TEST(13) TEST16(5, 6)
        SET_TEST(14) TEST16(6, 0)

        //test 15: r0-6l/r0-6h
        SET_TEST(15) TEST8(0, 1)
        SET_TEST(16) TEST8(1, 2)
        SET_TEST(17) TEST8(2, 3)
        SET_TEST(18) TEST8(3, 4)
        SET_TEST(19) TEST8(4, 5)
        SET_TEST(20) TEST8(5, 6)
        SET_TEST(21) TEST8(6, 0)

        "mov #0, r0\n"
        "mov r0, %[res]\n"
        "1:\n"
        COPY_REGS
        : [res] "+m" (res), REGS_VARS
        : [mem0] "i" (&testData[0]), [mem1] "i" (&testData[1]), [mem2] "i" (&testData[2])
        : "er0", "er1", "er2", "er3", "er4", "er5", "er6", "cc"
    );

    #undef TEST32
    #undef TEST16
    #undef TEST8

    if(res == 0)
        writeString(".\n");
    else
    {
        char buf[50];
        snprintf(buf, sizeof(buf), " failed at %i. ", res);
        writeString(buf);
        dumpRegs(regs);
        writeString("\n");
    }
}

void movIndirectDecTest()
{
    writeString("MOV INDIRECT DEC");

    int res = 1;
    long regs[8];
    //char cc;

    // writes through decrementing reg

    #define CHECK_MEM_EQUAL(reg, aReg, mem, val) \
        "mov %["#mem"], er"#aReg"\n" \
        "mov @er"#aReg", er"#reg"\n" \
        CHECK_EQUAL(reg, val)

    #define TEST32(reg, aReg) \
        "mov %[mem3], er"#aReg"\n" \
        \
        "mov #0, er"#reg"\n" \
        "ldc #0x80, ccr\n" \
        "mov er"#reg", @-er"#aReg"\n" \
        CHECK_COND_nZvc \
        \
        "mov #0x11223344, er"#reg"\n" \
        "ldc #0x83, ccr\n" \
        "mov er"#reg", @-er"#aReg"\n" \
        CHECK_COND_nzvC \
        \
        "mov #0x88776655, er"#reg"\n" \
        "mov er"#reg", @-er"#aReg"\n" \
        CHECK_COND_NzvC \
        /*now read back*/\
        CHECK_MEM_EQUAL(reg, aReg, mem2, 0) \
        CHECK_MEM_EQUAL(reg, aReg, mem1, 0x11223344) \
        CHECK_MEM_EQUAL(reg, aReg, mem0, 0x88776655)

    // En and Rn
    #define TEST16(reg, aReg) \
        "mov %[mem3], er"#aReg"\n" \
        \
        "mov #0, r"#reg"\n" \
        "ldc #0x80, ccr\n" \
        "mov r"#reg", @-er"#aReg"\n" \
        CHECK_COND_nZvc \
        \
        "mov #0x2244, r"#reg"\n" \
        "ldc #0x83, ccr\n" \
        "mov r"#reg", @-er"#aReg"\n" \
        CHECK_COND_nzvC \
        \
        "mov #0x8866, r"#reg"\n" \
        "mov r"#reg", @-er"#aReg"\n" \
        CHECK_COND_NzvC \
        \
        \
        "mov #0, e"#reg"\n" \
        "ldc #0x80, ccr\n" \
        "mov e"#reg", @-er"#aReg"\n" \
        CHECK_COND_nZvc \
        \
        "mov #0x3355, e"#reg"\n" \
        "ldc #0x83, ccr\n" \
        "mov e"#reg", @-er"#aReg"\n" \
        CHECK_COND_nzvC \
        \
        "mov #0x9977, e"#reg"\n" \
        "mov e"#reg", @-er"#aReg"\n" \
        CHECK_COND_NzvC \
        /*now read back*/\
        CHECK_MEM_EQUAL(reg, aReg, mem2, 0x22440000) \
        CHECK_MEM_EQUAL(reg, aReg, mem1, 0x00008866) \
        CHECK_MEM_EQUAL(reg, aReg, mem0, 0x99773355)

    // RnH and RnL
    #define TEST8(reg, aReg) \
        "mov %[mem2], er"#aReg"\n" \
        \
        "mov #0, r"#reg"l\n" \
        "ldc #0x80, ccr\n" \
        "mov r"#reg"l, @-er"#aReg"\n" \
        CHECK_COND_nZvc \
        \
        "mov #0x44, r"#reg"l\n" \
        "ldc #0x83, ccr\n" \
        "mov r"#reg"l, @-er"#aReg"\n" \
        CHECK_COND_nzvC \
        \
        "mov #0x88, r"#reg"l\n" \
        "mov r"#reg"l, @-er"#aReg"\n" \
        CHECK_COND_NzvC \
        \
        \
        "mov #0, r"#reg"h\n" \
        "ldc #0x80, ccr\n" \
        "mov r"#reg"h, @-er"#aReg"\n" \
        CHECK_COND_nZvc \
        \
        "mov #0x33, r"#reg"h\n" \
        "ldc #0x83, ccr\n" \
        "mov r"#reg"h, @-er"#aReg"\n" \
        CHECK_COND_nzvC \
        \
        "mov #0x99, r"#reg"h\n" \
        "mov r"#reg"h, @-er"#aReg"\n" \
        CHECK_COND_NzvC \
        /*now read back*/\
        CHECK_MEM_EQUAL(reg, aReg, mem1, 0x00884400) \
        CHECK_MEM_EQUAL(reg, aReg, mem0, 0x99779933)


    asm volatile(
        //test 1-7: er0-6
        TEST32(0, 1)
        SET_TEST(2) TEST32(1, 2)
        SET_TEST(3) TEST32(2, 3)
        SET_TEST(4) TEST32(3, 4)
        SET_TEST(5) TEST32(4, 5)
        SET_TEST(6) TEST32(5, 6)
        SET_TEST(7) TEST32(6, 0)


        //test 8-14: r0-6/e0-6
        SET_TEST(8) TEST16(0, 1)
        SET_TEST(9) TEST16(1, 2)
        SET_TEST(10) TEST16(2, 3)
        SET_TEST(11) TEST16(3, 4)
        SET_TEST(12) TEST16(4, 5)
        SET_TEST(13) TEST16(5, 6)
        SET_TEST(14) TEST16(6, 0)

        //test 15: r0-6l/r0-6h
        SET_TEST(15) TEST8(0, 1)
        SET_TEST(16) TEST8(1, 2)
        SET_TEST(17) TEST8(2, 3)
        SET_TEST(18) TEST8(3, 4)
        SET_TEST(19) TEST8(4, 5)
        SET_TEST(20) TEST8(5, 6)
        SET_TEST(21) TEST8(6, 0)

        "mov #0, r0\n"
        "mov r0, %[res]\n"
        "1:\n"
        COPY_REGS
        : [res] "+m" (res), REGS_VARS
        : [mem0] "i" (&testData[0]), [mem1] "i" (&testData[1]), [mem2] "i" (&testData[2]), [mem3] "i" (&testData[3])
        : "er0", "er1", "er2", "er3", "er4", "er5", "er6", "cc"
    );

    #undef TEST32
    #undef TEST16
    #undef TEST8

    if(res == 0)
        writeString(".\n");
    else
    {
        char buf[50];
        snprintf(buf, sizeof(buf), " failed at %i. ", res);
        writeString(buf);
        dumpRegs(regs);
        writeString("\n");
    }
}

void movAbsoluteTest()
{
    writeString("MOV ABS:32");

    int res = 1;
    long regs[8];
    //char cc;

    // indirect test, but with absolute addresses

    #define MOV_AND_TEST_COND(reg, cond) \
        "mov "#reg", %[mem]:32\n"\
        cond \
        "mov %[mem]:32, "#reg"\n"\
        cond

    #define TEST32(reg) \
        "mov #0, er"#reg"\n" \
        "ldc #0x80, ccr\n" \
        MOV_AND_TEST_COND(er##reg, CHECK_COND_nZvc) \
        CHECK_EQUAL(reg, 0) \
        \
        "mov #0x11223344, er"#reg"\n" \
        "ldc #0x83, ccr\n" \
        MOV_AND_TEST_COND(er##reg, CHECK_COND_nzvC) \
        CHECK_EQUAL(reg, 0x11223344) \
        \
        "mov #0x88776655, er"#reg"\n" \
        MOV_AND_TEST_COND(er##reg, CHECK_COND_Nzvc) \
        CHECK_EQUAL(reg, 0x88776655)

    // En and Rn
    #define TEST16(reg) \
        "mov #0x88776655, er"#reg"\n" \
        \
        "mov #0, r"#reg"\n" \
        "ldc #0x80, ccr\n" \
        MOV_AND_TEST_COND(r##reg, CHECK_COND_nZvc) \
        CHECK_EQUAL(reg, 0x88770000) \
        \
        "mov #0x2211, r"#reg"\n" \
        "ldc #0x83, ccr\n" \
        MOV_AND_TEST_COND(r##reg, CHECK_COND_nzvC) \
        CHECK_EQUAL(reg, 0x88772211) \
        \
        "mov #0x99AA, r"#reg"\n" \
        MOV_AND_TEST_COND(r##reg, CHECK_COND_Nzvc) \
        CHECK_EQUAL(reg, 0x887799AA) \
        \
        \
        "mov #0, e"#reg"\n" \
        "ldc #0x80, ccr\n" \
        MOV_AND_TEST_COND(e##reg, CHECK_COND_nZvc) \
        CHECK_EQUAL(reg, 0x000099aa) \
        \
        "mov #0x2211, e"#reg"\n" \
        "ldc #0x83, ccr\n" \
        MOV_AND_TEST_COND(e##reg, CHECK_COND_nzvC) \
        CHECK_EQUAL(reg, 0x221199AA) \
        \
        "mov #0xBBCC, e"#reg"\n" \
        MOV_AND_TEST_COND(e##reg, CHECK_COND_Nzvc) \
        CHECK_EQUAL(reg, 0xBBCC99AA)

    // RnH and RnL
    #define TEST8(reg) \
        "mov #0xBBCC99AA, er"#reg"\n" \
        \
        "mov #0, r"#reg"l\n" \
        "ldc #0x80, ccr\n" \
        MOV_AND_TEST_COND(r##reg##l, CHECK_COND_nZvc) \
        CHECK_EQUAL(reg, 0xBBCC9900) \
        \
        "mov #0x11, r"#reg"l\n" \
        "ldc #0x83, ccr\n" \
        MOV_AND_TEST_COND(r##reg##l, CHECK_COND_nzvC) \
        CHECK_EQUAL(reg, 0xBBCC9911) \
        \
        "mov #0x88, r"#reg"l\n" \
        MOV_AND_TEST_COND(r##reg##l, CHECK_COND_Nzvc) \
        CHECK_EQUAL(reg, 0xBBCC9988) \
        \
        \
        "mov #0, r"#reg"h\n" \
        "ldc #0x80, ccr\n" \
        MOV_AND_TEST_COND(r##reg##h, CHECK_COND_nZvc) \
        CHECK_EQUAL(reg, 0xBBCC0088) \
        \
        "mov #0x11, r"#reg"h\n" \
        "ldc #0x83, ccr\n" \
        MOV_AND_TEST_COND(r##reg##h, CHECK_COND_nzvC) \
        CHECK_EQUAL(reg, 0xBBCC1188) \
        \
        "mov #0x99, r"#reg"h\n" \
        MOV_AND_TEST_COND(r##reg##h, CHECK_COND_Nzvc) \
        CHECK_EQUAL(reg, 0xBBCC9988)


    asm volatile(
        //test 1-7: er0-6
        TEST32(0)
        SET_TEST(2) TEST32(1)
        SET_TEST(3) TEST32(2)
        SET_TEST(4) TEST32(3)
        SET_TEST(5) TEST32(4)
        SET_TEST(6) TEST32(5)
        SET_TEST(7) TEST32(6)


        //test 8-14: r0-6/e0-6
        SET_TEST(8) TEST16(0)
        SET_TEST(9) TEST16(1)
        SET_TEST(10) TEST16(2)
        SET_TEST(11) TEST16(3)
        SET_TEST(12) TEST16(4)
        SET_TEST(13) TEST16(5)
        SET_TEST(14) TEST16(6)

        //test 15: r0-6l/r0-6h
        SET_TEST(15) TEST8(0)
        SET_TEST(16) TEST8(1)
        SET_TEST(17) TEST8(2)
        SET_TEST(18) TEST8(3)
        SET_TEST(19) TEST8(4)
        SET_TEST(20) TEST8(5)
        SET_TEST(21) TEST8(6)

        "mov #0, r0\n"
        "mov r0, %[res]\n"
        "1:\n"
        COPY_REGS
        : [res] "+m" (res), REGS_VARS
        : [mem] "m" (testData)
        : "er0", "er1", "er2", "er3", "er4", "er5", "er6", "cc"
    );

    if(res == 0)
        writeString(".\n");
    else
    {
        char buf[50];
        snprintf(buf, sizeof(buf), " failed at %i. ", res);
        writeString(buf);
        dumpRegs(regs);
        writeString("\n");
    }

    writeString("MOV ABS:16");

    #undef MOV_AND_TEST_COND
    #define MOV_AND_TEST_COND(reg, cond) \
        "mov "#reg", %[mem]:16\n"\
        cond \
        "mov %[mem]:16, "#reg"\n"\
        cond

    asm volatile(
        //test 1-7: er0-6
        SET_TEST(1) TEST32(0)
        SET_TEST(2) TEST32(1)
        SET_TEST(3) TEST32(2)
        SET_TEST(4) TEST32(3)
        SET_TEST(5) TEST32(4)
        SET_TEST(6) TEST32(5)
        SET_TEST(7) TEST32(6)


        //test 8-14: r0-6/e0-6
        SET_TEST(8) TEST16(0)
        SET_TEST(9) TEST16(1)
        SET_TEST(10) TEST16(2)
        SET_TEST(11) TEST16(3)
        SET_TEST(12) TEST16(4)
        SET_TEST(13) TEST16(5)
        SET_TEST(14) TEST16(6)

        //test 15: r0-6l/r0-6h
        SET_TEST(15) TEST8(0)
        SET_TEST(16) TEST8(1)
        SET_TEST(17) TEST8(2)
        SET_TEST(18) TEST8(3)
        SET_TEST(19) TEST8(4)
        SET_TEST(20) TEST8(5)
        SET_TEST(21) TEST8(6)

        "mov #0, r0\n"
        "mov r0, %[res]\n"
        "1:\n"
        COPY_REGS
        : [res] "+m" (res), REGS_VARS
        : [mem] "m" (stackTestData)
        : "er0", "er1", "er2", "er3", "er4", "er5", "er6", "cc"
    );
    if(res == 0)
        writeString(".\n");
    else
    {
        char buf[50];
        snprintf(buf, sizeof(buf), " failed at %i. ", res);
        writeString(buf);
        dumpRegs(regs);
        writeString("\n");
    }

    writeString("MOV ABS:8");

    #undef MOV_AND_TEST_COND
    #define MOV_AND_TEST_COND(reg, cond) \
        "mov "#reg", %[mem]:8\n"\
        cond \
        "mov %[mem]:8, "#reg"\n"\
        cond

    asm volatile(
        //test 1-7: r0-6l/r0-6h
        SET_TEST(1) TEST8(0)
        SET_TEST(2) TEST8(1)
        SET_TEST(3) TEST8(2)
        SET_TEST(4) TEST8(3)
        SET_TEST(5) TEST8(4)
        SET_TEST(6) TEST8(5)
        SET_TEST(7) TEST8(6)

        "mov #0, r0\n"
        "mov r0, %[res]\n"
        "1:\n"
        COPY_REGS
        : [res] "+m" (res), REGS_VARS
        : [mem] "m" (TCORA1) // using a timer constant register
        : "er0", "er1", "er2", "er3", "er4", "er5", "er6", "cc"
    );

    #undef TEST32
    #undef TEST16
    #undef TEST8
    #undef MOV_AND_TEST_COND

    if(res == 0)
        writeString(".\n");
    else
    {
        char buf[50];
        snprintf(buf, sizeof(buf), " failed at %i. ", res);
        writeString(buf);
        dumpRegs(regs);
        writeString("\n");
    }
}

void pushPopTest()
{
    // these are aliases of mov with inc/dec on er7
    // not bothering to check flags here (checked the other seven regs)
    writeString("PUSH/POP");

    int res = 1;
    long regs[8];
    //char cc;


    asm volatile(

        "mov er7, er1\n"

        // push some stuff
        "mov #0x00112233, er2\n"
        "push er2\n"

        "mov #0x4455, r2\n"
        "push r2\n"

        "mov #0x6677, e2\n"
        "push e2\n"

        // pop it in a different order
        "pop er2\n"
        CHECK_EQUAL(2, 0x66774455)
        "pop r2\n"
        "pop e2\n"
        CHECK_EQUAL(2, 0x22330011)

        // make sure stack is where we started
        "cmp er1, er7\n"
        "bne 1f\n"

        // success
        "mov #0, r0\n"
        "mov r0, %[res]\n"

        "1:\n"
        "mov er7, er3\n"
        "mov er1, er7\n" // restore sp
        COPY_REGS
        : [res] "+m" (res), REGS_VARS
        :
        : "er0", "er1", "er2", "er3", "er4", "er5", "er6", "cc"
    );

    #undef TEST32
    #undef TEST16
    #undef TEST8

    if(res == 0)
        writeString(".\n");
    else
    {
        char buf[50];
        snprintf(buf, sizeof(buf), " failed at %i. ", res);
        writeString(buf);
        dumpRegs(regs);
        writeString("\n");
    }
}

