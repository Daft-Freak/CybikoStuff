#ifndef USBREGISTERS_H
#define USBREGISTERS_H

#include <stdint.h>

#define USB_DATA *reinterpret_cast<volatile uint8_t *>(0x200000)
#define USB_ADDR *reinterpret_cast<volatile uint8_t *>(0x200003)

enum class USBReg : uint8_t
{
    MCNTRL = 0,  // main control
    CCONF,       // clock configuration
    // 0x2 reserved
    RID = 0x3,   // revision identifier
    FAR,         // function address
    NFSR,        // node functional state
    MAEV,        // main event
    MAMSK,       // main mask
    ALTEV,       // alternate event
    ALTMSK,      // alternate mask
    TXEV,        // transmit event
    TXMSK,       // transmit mask
    RXEV,        // receive event
    RXMSK,       // receive mask
    NAKEV,       // NAK event
    NAKMSK,      // NAK mask
    FWEV,        // FIFO warning event
    FWMSK,       // FIFO warning mask
    FNH,         // frame number high byte
    FNL,         // frame number low byte
    DMACNTRL,    // DMA control
    DMAEV,       // DMA event
    DMAMSK,      // DMA mask
    MIR,         // mirror
    DMACNT,      // DMA count
    DMAERR,      // DMA error count
    // 0x1A reserved
    WKUP = 0x1B, // wake up
    // 0x1C - 0x1F reserved

    EPC0 = 0x20, // endpoint control 0
    TXD0,        // transmit data 0
    TXS0,        // transmit status 0
    TXC0,        // transmit command 0
    // 0x24 reserved
    RXD0 = 0x25, // receive data 0
    RXS0,        // receive status 0
    RXC0,        // receive command 0

    EPC1,        // endpoint control 1
    TXD1,        // transmit data 1
    TXS1,        // transmit status 1
    TXC1,        // transmit command 1
    EPC2,        // endpoint control 2
    RXD1,        // receive data 1
    RXS1,        // receive status 1
    RXC1,        // receive command 1

    EPC3,        // endpoint control 3
    TXD2,        // transmit data 2
    TXS2,        // transmit status 2
    TXC2,        // transmit command 2
    EPC4,        // endpoint control 4
    RXD2,        // receive data 2
    RXS2,        // receive status 2
    RXC2,        // receive command 2

    EPC5,        // endpoint control 5
    TXD3,        // transmit data 3
    TXS3,        // transmit status 3
    TXC3,        // transmit command 3
    EPC6,        // endpoint control 6
    RXD3,        // receive data 3
    RXS3,        // receive status 3
    RXC3,        // receive command 3
};

enum MCNTRLBits
{
    MCNTRL_SRST    = 1 << 0, // software reset
    // bit 2 reserved
    MCNTRL_VGE     = 1 << 2, // regulator enable
    MCNTRL_NAT     = 1 << 3, // node attached
    // bits 4-5 reserved
    MCNTRL_INTOC0  = 1 << 6,
    MCNTRL_INTOC1  = 1 << 7,
};

#define FAR_ADDR_MASK 0x7F

enum FARBits
{
    // bits 0-6 are the address
    FAR_AD_EN = 1 << 7
};

enum class NFS : uint8_t
{
    Reset = 0,
    Resume,
    Operational,
    Suspend
};

enum MAEVBits // also MAMSK
{
    MAEV_WARN  = 1 << 0,
    MAEV_ALT   = 1 << 1,
    MAEV_TX    = 1 << 2,
    MAEV_FRAME = 1 << 3,
    MAEV_NAK   = 1 << 4,
    MAEV_ULD   = 1 << 5,
    MAEV_RX    = 1 << 6,
    MAEV_INTR  = 1 << 7
};

enum ALTEVBits // also ALTMSK
{
    // bit 0 reserved
    ALTEV_WKUP   = 1 << 1,
    ALTEV_DMA    = 1 << 2,
    ALTEV_EOP    = 1 << 3,
    ALTEV_SD3    = 1 << 4,
    ALTEV_SD5    = 1 << 5,
    ALTEV_RESET  = 1 << 6,
    ALTEV_RESUME = 1 << 7
};

enum TXEVBits // also TXMSK
{
    TXEV_FIFO0  = 1 << 0,
    TXEV_FIFO1  = 1 << 1,
    TXEV_FIFO2  = 1 << 2,
    TXEV_FIFO3  = 1 << 3,
    TXEV_UDRRN0 = 1 << 4,
    TXEV_UDRRN1 = 1 << 5,
    TXEV_UDRRN2 = 1 << 6,
    TXEV_UDRRN3 = 1 << 7
};

enum RXEVBits // also RXMSK
{
    RXEV_FIFO0  = 1 << 0,
    RXEV_FIFO1  = 1 << 1,
    RXEV_FIFO2  = 1 << 2,
    RXEV_FIFO3  = 1 << 3,
    RXEV_OVRRN0 = 1 << 4,
    RXEV_OVRRN1 = 1 << 5,
    RXEV_OVRRN2 = 1 << 6,
    RXEV_OVRRN3 = 1 << 7
};

enum NAKEVBits // also NAKMSK
{
    NAKEV_IN0  = 1 << 0,
    NAKEV_IN1  = 1 << 1,
    NAKEV_IN2  = 1 << 2,
    NAKEV_IN3  = 1 << 3,
    NAKEV_OUT0 = 1 << 4,
    NAKEV_OUT1 = 1 << 5,
    NAKEV_OUT2 = 1 << 6,
    NAKEV_OUT3 = 1 << 7,
};

// fwev
// dmacntrl
// dmaev
// wkup

#define EPC_ADDR_MASK 0xF

enum EPC0Bits
{
    // bits 0-3 are the address (0)
    // bits 4-5 reserved
    EPC0_DEF   = 1 << 6,
    EPC0_STALL = 1 << 7
};

#define TXS_COUNT_MASK 0x1F

enum TXS0Bits
{
    // bits 0-4 are the count
    TXS0_DONE     = 1 << 5,
    TXS0_ACK_STAT = 1 << 6
    // bit 7 reserved
};

enum TXC0Bits
{
    TXC0_EN     = 1 << 0,
    // bit 1 reserved
    TXC0_TOGGLE = 1 << 2,
    TXC0_FLUSH  = 1 << 3,
    TXC0_IGN_IN = 1 << 4,
    // bits 5-7 reserved
};

#define RXS_COUNT_MASK 0xF

enum RXS0Bits
{
    // bits 0-3 are the count
    RXS0_LAST   = 1 << 4,
    RXS0_TOGGLE = 1 << 5,
    RXS0_SETUP  = 1 << 6,
    // bit 7 reserved
};

enum RXC0Bits
{
    RXC0_EN        = 1 << 0,
    RXC0_IGN_OUT   = 1 << 1,
    RXC0_IGN_SETUP = 1 << 2,
    RXC0_FLUSH     = 1 << 3,
    // bits 4-7 reserved
};
#endif