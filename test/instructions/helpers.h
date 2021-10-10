#pragma once

// assumes a "res" output
#define SET_TEST(n) \
    "mov #"#n", r0\n" \
    "mov r0, %[res]\n"

// check condition flags (upper case = set, lower = cleared), assumes a 1: label
#define CHECK_COND_NZVC "bpl 1f:16\n bne 1f:16\n bvc 1f:16\n bcc 1f:16\n"

#define CHECK_COND_NZVc "bpl 1f:16\n bne 1f:16\n bvc 1f:16\n bcs 1f:16\n"
#define CHECK_COND_Nzvc "bpl 1f:16\n beq 1f:16\n bvs 1f:16\n bcs 1f:16\n"
#define CHECK_COND_NzVC "bpl 1f:16\n beq 1f:16\n bvc 1f:16\n bcc 1f:16\n"
#define CHECK_COND_NzVc "bpl 1f:16\n beq 1f:16\n bvc 1f:16\n bcs 1f:16\n"
#define CHECK_COND_NzvC "bpl 1f:16\n beq 1f:16\n bvs 1f:16\n bcc 1f:16\n"
#define CHECK_COND_nZvc "bmi 1f:16\n bne 1f:16\n bvs 1f:16\n bcs 1f:16\n"
#define CHECK_COND_nzVc "bmi 1f:16\n beq 1f:16\n bvc 1f:16\n bcs 1f:16\n"
#define CHECK_COND_nzvC "bmi 1f:16\n beq 1f:16\n bvs 1f:16\n bcc 1f:16\n"

#define CHECK_COND_nzvc "bmi 1f:16\n beq 1f:16\n bvs 1f:16\n bcs 1f:16\n"

#define CHECK_EQUAL(reg, val) "cmp #"#val", er"#reg"\n bne 1f:16\n"

// copies regs to memory, assumes only one output before
#define REGS_VARS "=m" (regs[0]), "=m" (regs[1]), "=m" (regs[2]), "=m" (regs[3]), "=m" (regs[4]), "=m" (regs[5]), "=m" (regs[6]), "=m" (regs[7])
#define COPY_REGS "mov er0, %1\nmov er1, %2\nmov er2, %3\nmov er3, %4\nmov er4, %5\nmov er5, %6\nmov er6, %7\nmov er7, %8"

extern unsigned long testData[4];
extern unsigned long stackTestData[4];

void writeString(const char *str);
void dumpRegs(long er[8]);
