//2019-01-09

#pragma once

#include <stdint.h>

// helpers
// 0xFFFF0000 generates better code for bit ops...
#define REG8(x) *reinterpret_cast<volatile uint8_t *>(0xFFFF0000 | (x))
#define REG16(x) *reinterpret_cast<volatile uint16_t *>(0xFFFF0000 | (x))
#define REG32(x) *reinterpret_cast<volatile uint32_t *>(0xFFFF0000 | (x))

// port direction
#define PIN_BIT(n) (1 << n)

#define P1DDR REG8(0xFEB0)
#define P2DDR REG8(0xFEB1)
#define P3DDR REG8(0xFEB2)
// port 4 is read only
#define P5DDR REG8(0xFEB4)
#define P6DDR REG8(0xFEB5)
#define PADDR REG8(0xFEB9)
#define PBDDR REG8(0xFEBA)
#define PCDDR REG8(0xFEBB)
#define PDDDR REG8(0xFEBC)
#define PEDDR REG8(0xFEBD)
#define PFDDR REG8(0xFEBE)
#define PGDDR REG8(0xFEBF)

// dma controller
enum DMABCRBits
{
    DMABCR_DTIE0A     = 1 << 0,
    DMABCR_DTIE0B     = 1 << 1,
    DMABCR_DTIE1A     = 1 << 2,
    DMABCR_DTIE1B     = 1 << 3,

    DMABCR_FAE0       = 1 << 14,
    DMABCR_FAE1       = 1 << 15,

    // full address mode
    DMABCR_F_DTE0     = 1 << 4,
    DMABCR_F_DTME0    = 1 << 5,
    DMABCR_F_DTE1     = 1 << 6,
    DMABCR_F_DTME1    = 1 << 7,

    DMABCR_F_DTA0     = 1 << 9,

    DMABCR_F_DTA1     = 1 << 11,

    // short address mode
    DMABCR_S_DTE0A    = 1 << 4,
    DMABCR_S_DTE0B    = 1 << 5,
    DMABCR_S_DTE1A    = 1 << 6,
    DMABCR_S_DTE1B    = 1 << 7,
    DMABCR_S_DTA0A    = 1 << 8,
    DMABCR_S_DTA0B    = 1 << 9,
    DMABCR_S_DTA1A    = 1 << 10,
    DMABCR_S_DTA1B    = 1 << 11,
    DMABCR_S_SAE0     = 1 << 12,
    DMABCR_S_SAE1     = 1 << 13,
};

// full address
enum DMACRABits
{
    DMACRA_BLKE   = 1 << 3,
    DMACRA_BLKDIR = 1 << 4,
    DMACRA_SAIDE  = 1 << 5,
    DMACRA_SAID   = 1 << 6,
    DMACRA_DTSZ   = 1 << 7
};

enum DMACRBBits
{
    DMACRB_DTF0   = 1 << 0,
    DMACRB_DTF1   = 1 << 1,
    DMACRB_DTF2   = 1 << 2,
    DMACRB_DTF3   = 1 << 3,

    DMACRB_DAIDE  = 1 << 5,
    DMACRB_DAID   = 1 << 6,
};

// short address
enum DMACRBits
{
    DMACR_DTF0  = 1 << 0,
    DMACR_DTF1  = 1 << 1,
    DMACR_DTF2  = 1 << 2,
    DMACR_DTF3  = 1 << 3,
    DMACR_DTDIR = 1 << 4,
    DMACR_RPE   = 1 << 5,
    DMACR_DTID  = 1 << 6,
    DMACR_DTSZ  = 1 << 7
};

#define MAR0A   REG32(0xFEE0)
#define IOAR0A  REG16(0xFEE4)
#define ETCR0A  REG16(0xFEE6)
#define MAR0B   REG32(0xFEE8)
#define IOAR0B  REG16(0xFEEC)
#define ETCR0B  REG16(0xFEEE)
#define MAR1A   REG32(0xFEF0)
#define IOAR1A  REG16(0xFEF4)
#define ETCR1A  REG16(0xFEF6)
#define MAR1B   REG32(0xFEF8)
#define IOAR1B  REG16(0xFEFC)
#define ETCR1B  REG16(0xFEFE)
#define DMAWER  REG8(0xFF00)
#define DMATCR  REG8(0xFF01)
#define DMACR0A REG8(0xFF02)
#define DMACR0B REG8(0xFF03)
#define DMACR1A REG8(0xFF04)
#define DMACR1B REG8(0xFF05)
#define DMABCR  REG16(0xFF06)

// interrupt controller

#define IRQ_BIT(n) (1 << n)

#define ISCR REG16(0xFF2C)
//#define ISCRL REG8(0xFF2C)
//#define ISCRH REG8(0xFF2D)
#define IER REG8(0xFF2E)
#define ISR REG8(0xFF2F)

// module stop control
enum MSTPCRBits
{
    MSTPCR_SCI0    = 1 << 5,
    MSTPCR_SCI1    = 1 << 6,
    MSTPCR_SCI2    = 1 << 7,

    MSTPCR_A2D     = 1 << 9,  // A/D
    MSTPCR_D2A     = 1 << 10, // D/A
    MSTPCR_PPG     = 1 << 11,
    MSTPCR_Timer8b = 1 << 12, // 8-bit timer
    MSTPCR_TPU     = 1 << 13,
    MSTPCR_DTC     = 1 << 14,
    MSTPCR_DMAC    = 1 << 15
};

#define MSTPCR REG16(0xFF3C)

// ports

#define PORT1 REG8(0xFF50)
#define PORT2 REG8(0xFF51)
#define PORT3 REG8(0xFF52)
#define PORT4 REG8(0xFF53)
#define PORT5 REG8(0xFF54)
#define PORT6 REG8(0xFF55)
#define PORTA REG8(0xFF59)
#define PORTB REG8(0xFF5A)
#define PORTC REG8(0xFF5B)
#define PORTD REG8(0xFF5C)
#define PORTE REG8(0xFF5D)
#define PORTF REG8(0xFF5E)
#define PORTG REG8(0xFF5F)

// port data

#define P1DR REG8(0xFF60)
#define P2DR REG8(0xFF61)
#define P3DR REG8(0xFF62)
// port 4 is read only
#define P5DR REG8(0xFF64)
#define P6DR REG8(0xFF65)
#define PADR REG8(0xFF69)
#define PBDR REG8(0xFF6A)
#define PCDR REG8(0xFF6B)
#define PDDR REG8(0xFF6C)
#define PEDR REG8(0xFF6D)
#define PFDR REG8(0xFF6E)
#define PGDR REG8(0xFF6F)

// serial

enum SMRBits
{
    // serial mode
    SMR_CKS0 = 1 << 0,
    SMR_CKS1 = 1 << 1,
    SMR_MP   = 1 << 2,
    SMR_STOP = 1 << 3,
    SMR_OE   = 1 << 4,
    SMR_PE   = 1 << 5,
    SMR_CHR  = 1 << 6,
    SMR_CA   = 1 << 7

    // 2, 3, 6, 7 are different in smart card mode
};

enum SCRBits
{
    SCR_CKE0 = 1 << 0,
    SCR_CKE1 = 1 << 1,
    SCR_TEIE = 1 << 2,
    SCR_MPIE = 1 << 3,
    SCR_RE   = 1 << 4,
    SCR_TE   = 1 << 5,
    SCR_RIE  = 1 << 6,
    SCR_TIE  = 1 << 7
};

enum SSRBits
{
    SSR_MPBT = 1 << 0,
    SSR_MPB  = 1 << 1,
    SSR_TEND = 1 << 2,
    SSR_PER  = 1 << 3,
    SSR_FER  = 1 << 4,
    SSR_ORER = 1 << 5,
    SSR_RDRF = 1 << 6,
    SSR_TDRE = 1 << 7

    // 4 is different in smart card mode
};

enum SCMRBits
{
    SCMR_SMIF = 1 << 0,

    SMCR_SINV = 1 << 2,
    SCMR_SDIR = 1 << 3
};

#define SMR0  REG8(0xFF78)
#define BRR0  REG8(0xFF79)
#define SCR0  REG8(0xFF7A)
#define TDR0  REG8(0xFF7B)
#define SSR0  REG8(0xFF7C)
#define RDR0  REG8(0xFF7D)
#define SCMR0 REG8(0xFF7E)

#define SMR1  REG8(0xFF80)
#define BRR1  REG8(0xFF81)
#define SCR1  REG8(0xFF82)
#define TDR1  REG8(0xFF83)
#define SSR1  REG8(0xFF84)
#define RDR1  REG8(0xFF85)
#define SCMR1 REG8(0xFF86)

#define SMR2  REG8(0xFF88)
#define BRR2  REG8(0xFF89)
#define SCR2  REG8(0xFF8A)
#define TDR2  REG8(0xFF8B)
#define SSR2  REG8(0xFF8C)
#define RDR2  REG8(0xFF8D)
#define SCMR2 REG8(0xFF8E)

#define SMRx(x)  REG8(0xFF78 + x * 8)
#define BRRx(x)  REG8(0xFF79 + x * 8)
#define SCRx(x)  REG8(0xFF7A + x * 8)
#define TDRx(x)  REG8(0xFF7B + x * 8)
#define SSRx(x)  REG8(0xFF7C + x * 8)
#define RDRx(x)  REG8(0xFF7D + x * 8)
#define SCMRx(x) REG8(0xFF7E + x * 8)

// timer0/1

// TODO: define bits
#define TCR0 REG8(0xFFB0)
#define TCR1 REG8(0xFFB1)

enum TCSR01Bits
{
    TCSR_OS0   = 1 << 0,
    TCSR_OS1   = 1 << 1,
    TCSR_OS2   = 1 << 2,
    TCSR_OS3   = 1 << 3,
    TCSR0_ADTE = 1 << 4,
    TCSR_OVF   = 1 << 5,
    TCSR_CMFA  = 1 << 6,
    TCSR_CMFB  = 1 << 7,
};

#define TCSR0 REG8(0xFFB2)
#define TCSR1 REG8(0xFFB3)

#define TCORA0 REG8(0xFFB4)
#define TCORA1 REG8(0xFFB5)

#define TCORB0 REG8(0xFFB6)
#define TCORB1 REG8(0xFFB7)

#define TCNT0 REG8(0xFFB8)
#define TCNT1 REG8(0xFFB9)

