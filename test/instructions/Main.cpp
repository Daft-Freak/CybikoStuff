#include <stdio.h>
#include <string.h>

#include "Serial.hpp"

#include "helpers.h"

// in external ram, so 32bit address required
unsigned long testData[4];
unsigned long stackTestData[4] [[gnu::section(".stack")]]; // the stack is in on-chip ram, which can be accessed with a 16-bit address

void writeString(const char *str)
{
    sci1Write(reinterpret_cast<const uint8_t *>(str), strlen(str));
}

void dumpRegs(long er[8])
{
    char buf [8 * 9 + 6];
    snprintf(buf, sizeof(buf), "regs: %lx %lx %lx %lx %lx %lx %lx %lx", er[0], er[1], er[2], er[3], er[4], er[5], er[6], er[7]);
    writeString(buf);
}

void movImmTest();
void movRegTest();
void movIndirectTest();
void movIndirectDispTest();
void movIndirectIncTest();
void movIndirectDecTest();
void movAbsoluteTest();

void pushPopTest();

void ldmTest();
void stmTest();

void add32Test();
void add16Test();
void add8Test();

void sub32Test();
void sub16Test();
void sub8Test();

void cmp32Test();
void cmp16Test();
void cmp8Test();

void neg32Test();
void neg16Test();
void neg8Test();

void addxTest();
void subxTest();

void daaTest();
void dasTest();

void incTest();
void decTest();

void addsTest();
void subsTest();

void mulxu16Test();
void mulxu8Test();

void divxu16Test();
void divxu8Test();

void mulxs16Test();
void mulxs8Test();

void divxs16Test();
void divxs8Test();

void extu32Test();
void extu16Test();

void exts32Test();
void exts16Test();

void tasTest();

void and32Test();
void and16Test();
void and8Test();

void or32Test();
void or16Test();
void or8Test();

void xor32Test();
void xor16Test();
void xor8Test();

void not32Test();
void not16Test();
void not8Test();

void shal32Test();
void shal16Test();
void shal8Test();

void shar32Test();
void shar16Test();
void shar8Test();

void shll32Test();
void shll16Test();
void shll8Test();

void shlr32Test();
void shlr16Test();
void shlr8Test();

void rotl32Test();
void rotl16Test();
void rotl8Test();

void rotr32Test();
void rotr16Test();
void rotr8Test();

void rotxl32Test();
void rotxl16Test();
void rotxl8Test();

void rotxr32Test();
void rotxr16Test();
void rotxr8Test();

void bsetTest();
void bclrTest();
void bnotTest();
void btstTest();
void bldTest();
void bildTest();
void bstTest();
void bistTest();
void bandTest();
void biandTest();
void borTest();
void biorTest();
void bxorTest();
void bixorTest();

void branchTest();
// JMP, BSR, JSR, RTS

// TRAPA, RTE, SLEEP

// LDC/STC.B
void ldc16Test();
void stc16Test();

void andcTest();
void orcTest();
void xorcTest();

void nopTest()
{
    writeString("NOP");

    int res = 1;
    long regs[8];
    unsigned char cc;

    // very exciting test
    asm volatile(
        "ldc #0x80, ccr\n"
        "nop\n"
        CHECK_COND_nzvc     // CPU used NOP!

        SET_TEST(2)
        "ldc #0x8F, ccr\n"
        "nop\n"
        CHECK_COND_NZVC     // but nothing happened!

        SET_TEST(3)
        "mov #0x01234567, er0\n"
        "mov #0x89ABCDEF, er1\n"
        "mov #0xFEDCBA98, er2\n"
        "mov #0x76543210, er3\n"
        "nop\n"
        CHECK_EQUAL(0, 0x01234567) // in the end
        CHECK_EQUAL(1, 0x89ABCDEF) // the registers
        CHECK_EQUAL(2, 0xFEDCBA98) // refused
        CHECK_EQUAL(3, 0x76543210) // to change

        // success
        "mov #0, r0\n"
        "mov r0, %[res]\n"
        "1:\n"
        "stc ccr, %[cc]l\n"
        COPY_REGS
        : [res] "+m" (res), REGS_VARS, [cc] "=r" (cc)
        :
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

// totally didn't steal this from 32blit
static uint32_t prng_xorshift_state = 0xDAF71234;

static uint32_t prng_xorshift_next() {
    uint32_t x = prng_xorshift_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    prng_xorshift_state = x;
    return x;
}

void eepmovTest()
{
    static uint8_t buf1[500]{0};
    static uint8_t buf2[500]{0};

    // fill in test data
    for(auto &v : buf1)
        v = prng_xorshift_next();

    // .w allows interrupts, .b does not
    writeString("EEPMOV");

    // .B
    int res = 1;

    asm volatile(
        "mov #0x123456C8, er4\n"
        "mov %1, er5\n"
        "mov %2, er6\n"
        "eepmov.b\n"
        CHECK_EQUAL(4, 0x12345600)
        "cmp %3, er5\n bne 1f\n"
        "cmp %4, er6\n bne 1f\n"

        "mov #0, %[res]\n" // success
        "1:"
        : [res] "+r" (res)
        : "i" (buf1), "i" (buf2), "i" (buf1 + 0xC8), "i" (buf2 + 0xC8)
        : "er4", "er5", "er6"
    );

    char buf[50];
    if(res)
    {
        snprintf(buf, sizeof(buf), " failed at B (regs after)\n", res);
        writeString(buf);
        return;
    }

    // compare values
    int i = 0;
    for(; i < 0xC8; i++)
    {
        if(buf1[i] != buf2[i])
        {
            snprintf(buf, sizeof(buf), " mismatch at %i: %02X != %02X\n", i, buf1[i], buf2[i]);
            writeString(buf);
            return;
        }
    }
    for(; i < sizeof(buf2); i++)
    {
        if(buf2[i])
        {
            snprintf(buf, sizeof(buf), " mismatch at %i: %02X != 0\n", i, buf2[i]);
            writeString(buf);
            return;
        }
    }

    // .W
    res = 1;

    asm volatile(
        "mov #0x12340190, er4\n"
        "mov %1, er5\n"
        "mov %2, er6\n"
        "eepmov.w\n"
        CHECK_EQUAL(4, 0x12340000)
        "cmp %3, er5\n bne 1f\n"
        "cmp %4, er6\n bne 1f\n"

        "mov #0, %[res]\n" // success
        "1:"
        : [res] "+r" (res)
        : "i" (buf1), "i" (buf2), "i" (buf1 + 0x190), "i" (buf2 + 0x190)
        : "er4", "er5", "er6"
    );

    if(res)
    {
        snprintf(buf, sizeof(buf), " failed at W (regs after)\n", res);
        writeString(buf);
        return;
    }

    // compare values
    for(i = 0; i < 0x190; i++)
    {
        if(buf1[i] != buf2[i])
        {
            snprintf(buf, sizeof(buf), " mismatch at %i: %02X != %02X\n", i, buf1[i], buf2[i]);
            writeString(buf);
            return;
        }
    }
    for(; i < sizeof(buf2); i++)
    {
        if(buf2[i])
        {
            snprintf(buf, sizeof(buf), " mismatch at %i: %02X != 0\n", i, buf2[i]);
            writeString(buf);
            return;
        }
    }

    writeString(".\n");
}

int main()
{
    // sci1Init(); // rom leaves this initialised

    writeString("Starting...\n");

    char buf[40];
    volatile int i = 42;
    long l = long(i) << 16;
    snprintf(buf, sizeof(buf), "printf test %i %x %li %lx\n", i, i, l, l);
    writeString(buf);

    // so many mov tests
    movImmTest();
    movRegTest();
    movIndirectTest();
    movIndirectDispTest();
    movIndirectIncTest();
    movIndirectDecTest();
    movAbsoluteTest();

    // ... and now we stop testing every register

    pushPopTest();

    ldmTest();
    stmTest();

    add32Test();
    add16Test();
    add8Test();

    sub32Test();
    sub16Test();
    sub8Test();

    cmp32Test();
    cmp16Test();
    cmp8Test();

    neg32Test();
    neg16Test();
    neg8Test();

    addxTest();
    subxTest();

    daaTest();
    dasTest();

    incTest();
    decTest();

    addsTest();
    subsTest();

    mulxu16Test();
    mulxu8Test();

    divxu16Test();
    divxu8Test();

    mulxs16Test();
    mulxs8Test();

    divxs16Test();
    divxs8Test();

    extu32Test();
    extu16Test();

    exts32Test();
    exts16Test();

    tasTest();

    // do not have MAC

    and32Test();
    and16Test();
    and8Test();

    or32Test();
    or16Test();
    or8Test();

    xor32Test();
    xor16Test();
    xor8Test();

    not32Test();
    not16Test();
    not8Test();

    shal32Test();
    shal16Test();
    shal8Test();

    shar32Test();
    shar16Test();
    shar8Test();

    shll32Test();
    shll16Test();
    shll8Test();

    shlr32Test();
    shlr16Test();
    shlr8Test();

    rotl32Test();
    rotl16Test();
    rotl8Test();

    rotr32Test();
    rotr16Test();
    rotr8Test();

    rotxl32Test();
    rotxl16Test();
    rotxl8Test();

    rotxr32Test();
    rotxr16Test();
    rotxr8Test();

    bsetTest();
    bclrTest();
    bnotTest();
    btstTest();
    bldTest();
    bildTest();
    bstTest();
    bistTest();
    bandTest();
    biandTest();
    borTest();
    biorTest();
    bxorTest();
    bixorTest();

    branchTest(); // maybe this should be early as you can't really trust the other results if this fails...
    // JMP
    // BSR
    // JSR tests use functions
    // RTS so I guess these are tested? (maybe even BSR?)

    // TRAPA is unusable (vectors all 0)
    // RTE, SLEEP would need to trigger an interrupt to test
    // LDC/STC imm/reg used in most other tests
    ldc16Test();
    stc16Test();

    andcTest();
    orcTest();
    xorcTest();

    nopTest();

    eepmovTest();

    while(true)
    {
    }
    return 0;
}
