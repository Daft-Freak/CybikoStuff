// split out 20 dec 2019
// probably written in jan

#include <iostream>

#include "USBDevice.h"
#include "USBRegisters.h"

USBDevice::USBDevice()
{
    reset();
}

uint8_t USBDevice::read(uint32_t addr)
{
    if(addr & 0x1)
    {
        std::cerr << "Attempt to read USB address register!" << std::endl;
        return 0;
    }

    switch(static_cast<USBReg>(regAddr))
    {
        case USBReg::MAEV:
            return getMAEV();
        case USBReg::MAMSK:
            return mainMask;
        case USBReg::ALTMSK:
            return altMask;
        case USBReg::TXEV:
            return txEvent;
        case USBReg::TXMSK:
            return txMask;
        case USBReg::RXEV:
            return rxEvent;
        case USBReg::RXMSK:
            return rxMask;
        case USBReg::NAKEV:
        {
            auto tmp = nakEvent;
            nakEvent = 0; // cleared on read
            return tmp;
        }
        case USBReg::NAKMSK:
            return nakMask;

        case USBReg::RXD0:
        {
            uint8_t v = 0;
            if(controlFIFO.getFilled())
                v = controlFIFO.pop();

            return v;
        }

        // test hax
        case USBReg::TXS0:
        {
            // ack?
            bool done = (txEvent & TXEV_FIFO0);
            txEvent &= ~TXEV_FIFO0;
            return (done ? TXS0_DONE | TXS0_ACK_STAT : 0) | controlFIFO.getFilled();
        }

        case USBReg::RXS0:
            // reading status clears RXEV
            rxEvent &= ~RXEV_FIFO0;

            if(controlFIFO.getFilled() == 8)
                return RXS0_SETUP | 8;
            else
                return controlFIFO.getFilled();
        //
    }

    std::cout << "USB r " << std::hex << static_cast<int>(regAddr) << "(" << getRegName(regAddr) << ")" << std::dec << std::endl;

    return 0;
}

void USBDevice::write(uint32_t addr, uint8_t val)
{
    //handle address write
    if(addr & 0x1)
    {
        regAddr = val;
        return;
    }

    switch(static_cast<USBReg>(regAddr))
    {
        case USBReg::MCNTRL:
            if(val & MCNTRL_SRST)
                reset();

            if(val & 0xFE)
                std::cout << "USB w MCNTRL = " << std::hex << static_cast<int>(val) << std::dec << std::endl;
            break;

        case USBReg::MAMSK:
            mainMask = val;
            break;

        case USBReg::ALTMSK:
            altMask = val;
            break;

        case USBReg::TXMSK:
            txMask = val;
            break;

        case USBReg::RXMSK:
            rxMask = val;
            break;

        case USBReg::NAKMSK:
            nakMask = val;
            break;

        case USBReg::TXC0:
            txEnable[0] = (val & TXC0_EN) != 0;

            // TODO: handle unsent data if sending...
            if(val & TXC0_FLUSH)
                controlFIFO.reset();

            if(val & TXC0_IGN_IN)
                std::cout << "USB TXC0 IGN_IN\n";
            break;

        case USBReg::TXD0:
            if(controlFIFO.getFilled() < 8)
                controlFIFO.push(val);
            break;

        case USBReg::RXC0:
            rxEnable[0] = (val & RXC0_EN) != 0;

            if(val & RXC0_FLUSH)
                controlFIFO.reset();

            if(val & RXC0_IGN_OUT)
                std::cout << "USB RXC0 IGN_OUT\n";
            if(val & RXC0_IGN_SETUP)
                std::cout << "USB RXC0 IGN_OUT\n";

            break;
        default:
            std::cout << "USB w " << std::hex << static_cast<int>(regAddr) << "(" << getRegName(regAddr) << ") = " << static_cast<int>(val) << std::dec << std::endl;
    }

}

void USBDevice::update(H8CPU &cpu)
{
}

void USBDevice::reset()
{
    mainMask = 0;
    altMask = 0;
    txMask = 0;
    rxMask = 0;
    nakMask = 0;

    txEvent = 0;
    rxEvent = 0;
    nakEvent = 0;

    for(int i = 0; i < 4; i++)
        txEnable[i] = rxEnable[i] = false;

    controlFIFO.reset();
}

uint8_t USBDevice::getMAEV() const
{
    uint8_t v = 0;

    if(txEvent & txMask)
        v |= MAEV_TX;

    if(rxEvent & rxMask)
        v |= MAEV_RX;

    if(nakEvent & nakMask)
        v |= MAEV_NAK;

    return v;
}

const char *USBDevice::getRegName(int reg)
{
    const char *regNames[]
    {
        "MCNTRL",
        "CCONF",
        "", // 0x2 reserved
        "RID",
        "FAR",
        "NFSR",
        "MAEV",
        "MAMSK",
        "ALTEV",
        "ALTMSK",
        "TXEV",
        "TXMSK",
        "RXEV",
        "RXMSK",
        "NAKEV",
        "NAKMSK",
        "FWEV",
        "FWMSK",
        "FNH",
        "FNL",
        "DMACNTRL",
        "DMAEV",
        "DMAMSK",
        "MIR",
        "DMACNT",
        "DMAERR",
        "", // 0x1A reserved
        "WKUP",
        "", // 0x1C - 0x1F reserved
        "",
        "",
        "",
        "EPC0",
        "TXD0",
        "TXS0",
        "TXC0",
        "", // 0x24 reserved
        "RXD0",
        "RXS0",
        "RXC0",
        "EPC1",
        "TXD1",
        "TXS1",
        "TXC1",
        "EPC2",
        "RXD1",
        "RXS1",
        "RXC1",
        "EPC3",
        "TXD2",
        "TXS2",
        "TXC2",
        "EPC4",
        "RXD2",
        "RXS2",
        "RXC2",
        "EPC5",
        "TXD3",
        "TXS3",
        "TXC3",
        "EPC6",
        "RXD3",
        "RXS3",
        "RXC3"
    };

    if(reg >= 64)
        return "??";

    return regNames[reg];
}

