#include <stdio.h>

#include "Registers.hpp"

#include "helpers.h"

void branchTest()
{
    writeString("Bcc");

    int res = 1;
    unsigned char cc;

    testData[0] = 0;
    stackTestData[0] = 0;
    TCORA1 = 0;

    #define FAIL_IF_BRANCH_TAKEN(bcc) #bcc " 1f\n" #bcc " 1f:16\n"
    #define FAIL_IF_BRANCH_NOT_TAKEN(bcc) \
        #bcc " 2f\n" \
        "bra 1f\n" \
        "2:"\
        #bcc " 2f:16\n" \
        "bra 1f\n" \
        "2:"\

    // need to make sure short branches can reach...
    asm volatile(
        // always/never
        "bra 2f\n" // if this one doesn't work we can't do much about it...
        "sleep\n"
        "2:\n"
        "bra 2f:16\n"
        "sleep\n"
        "2:\n"
        FAIL_IF_BRANCH_TAKEN(brn)

        // flags set (branch not taken)
        SET_TEST(2)
        "ldc #0x80, ccr\n"
        FAIL_IF_BRANCH_TAKEN(bmi)
        FAIL_IF_BRANCH_TAKEN(beq)
        FAIL_IF_BRANCH_TAKEN(bvs)
        FAIL_IF_BRANCH_TAKEN(bcs)

        // flags clear (branch not taken)
        SET_TEST(3)
        "ldc #0x8F, ccr\n"
        FAIL_IF_BRANCH_TAKEN(bpl)
        FAIL_IF_BRANCH_TAKEN(bne)
        FAIL_IF_BRANCH_TAKEN(bvc)
        FAIL_IF_BRANCH_TAKEN(bcc)

        // flags set (branch taken)
        SET_TEST(4)
        "ldc #0x80, ccr\n"
        FAIL_IF_BRANCH_NOT_TAKEN(bpl)
        FAIL_IF_BRANCH_NOT_TAKEN(bne)
        FAIL_IF_BRANCH_NOT_TAKEN(bvc)
        FAIL_IF_BRANCH_NOT_TAKEN(bcc)

        // flags clear (branch taken)
        SET_TEST(5)
        "ldc #0x8F, ccr\n"
        "bra 2f\n1: bra 1f\n2:" // boing (+126 from first brn)
        FAIL_IF_BRANCH_NOT_TAKEN(bmi)
        FAIL_IF_BRANCH_NOT_TAKEN(beq)
        FAIL_IF_BRANCH_NOT_TAKEN(bvs)
        FAIL_IF_BRANCH_NOT_TAKEN(bcs)

        // BHI/BLS (Z | C)
        SET_TEST(6)
        "ldc #0x80, ccr\n"
        FAIL_IF_BRANCH_NOT_TAKEN(bhi)
        FAIL_IF_BRANCH_TAKEN(bls)
        "ldc #0x81, ccr\n"
        FAIL_IF_BRANCH_TAKEN(bhi)
        FAIL_IF_BRANCH_NOT_TAKEN(bls)
        "ldc #0x84, ccr\n"
        FAIL_IF_BRANCH_TAKEN(bhi)
        FAIL_IF_BRANCH_NOT_TAKEN(bls)
        "ldc #0x85, ccr\n"
        FAIL_IF_BRANCH_TAKEN(bhi)
        FAIL_IF_BRANCH_NOT_TAKEN(bls)

        // BGE/BLT (N ^ V)
        SET_TEST(7)
        "bra 2f\n1: bra 1f\n2:" // boing
        "ldc #0x80, ccr\n"
        FAIL_IF_BRANCH_NOT_TAKEN(bge)
        FAIL_IF_BRANCH_TAKEN(blt)
        "ldc #0x82, ccr\n"
        FAIL_IF_BRANCH_TAKEN(bge)
        FAIL_IF_BRANCH_NOT_TAKEN(blt)
        "ldc #0x88, ccr\n"
        FAIL_IF_BRANCH_TAKEN(bge)
        FAIL_IF_BRANCH_NOT_TAKEN(blt)
        "ldc #0x8A, ccr\n"
        FAIL_IF_BRANCH_NOT_TAKEN(bge)
        FAIL_IF_BRANCH_TAKEN(blt)

        // BGT/BLE (Z | (N ^ V))
        SET_TEST(8)
        "ldc #0x80, ccr\n"
        FAIL_IF_BRANCH_NOT_TAKEN(bgt)
        FAIL_IF_BRANCH_TAKEN(ble)
        "ldc #0x82, ccr\n"
        FAIL_IF_BRANCH_TAKEN(bgt)
        FAIL_IF_BRANCH_NOT_TAKEN(ble)
        "bra 2f\n1: bra 1f\n2:" // boing
        "ldc #0x84, ccr\n"
        FAIL_IF_BRANCH_TAKEN(bgt)
        FAIL_IF_BRANCH_NOT_TAKEN(ble)
        "ldc #0x88, ccr\n"
        FAIL_IF_BRANCH_TAKEN(bgt)
        FAIL_IF_BRANCH_NOT_TAKEN(ble)

        "ldc #0x86, ccr\n"
        FAIL_IF_BRANCH_TAKEN(bgt)
        FAIL_IF_BRANCH_NOT_TAKEN(ble)
        "ldc #0x8A, ccr\n"
        FAIL_IF_BRANCH_NOT_TAKEN(bgt)
        FAIL_IF_BRANCH_TAKEN(ble)
        "ldc #0x8C, ccr\n"
        FAIL_IF_BRANCH_TAKEN(bgt)
        FAIL_IF_BRANCH_NOT_TAKEN(ble)
        "ldc #0x8E, ccr\n"
        FAIL_IF_BRANCH_TAKEN(bgt)
        FAIL_IF_BRANCH_NOT_TAKEN(ble)

        // success
        "mov #0, r0\n"
        "mov r0, %[res]\n"
        "1:\n"
        "stc ccr, %[cc]l\n"
        : [res] "+m" (res), [cc] "=r" (cc)
        :
        : "cc"
    );

    #undef FAIL_IF_BRANCH_TAKEN
    #undef FAIL_IF_BRANCH_NOT_TAKEN

    if(res == 0)
        writeString(".\n");
    else
    {
        char buf[30];
        snprintf(buf, sizeof(buf), " failed at %i. ccr: %02x \n", res, cc);
        writeString(buf);
    }
}
