#include "Serial.hpp"
#include "IOPort.hpp"

void sci0Init(uint8_t smr, uint8_t scmr, uint8_t brr)
{
    setPortDirection(IOPort::_3, PIN_BIT(0) | PIN_BIT(4), IODir::Out); // tx/sck = out
    setPortDirection(IOPort::_3, PIN_BIT(2), IODir::In); // rx in

    SCR0 = 0;
    // turn it off and back on...
    MSTPCR |= MSTPCR_SCI0;
    MSTPCR &= ~MSTPCR_SCI0;

    SMR0 = smr;
    SCMR0 = scmr;
    BRR0 = brr;
}

void sci1Init(uint8_t smr, uint8_t scmr, uint8_t brr)
{
    setPortDirection(IOPort::_3, PIN_BIT(1) | PIN_BIT(5), IODir::Out); // tx/sck = out
    setPortDirection(IOPort::_3, PIN_BIT(3), IODir::In); // rx in

    SCR1 = 0;
    // turn it off and back on...
    MSTPCR |= MSTPCR_SCI1;
    MSTPCR &= ~MSTPCR_SCI1;

    SMR1 = smr;
    SCMR1 = scmr;
    BRR1 = brr;
}

void sci2Init(uint8_t smr, uint8_t scmr, uint8_t brr)
{
    setPortDirection(IOPort::_5, PIN_BIT(0) | PIN_BIT(2), IODir::Out); // tx/sck = out
    setPortDirection(IOPort::_5, PIN_BIT(1), IODir::In); // rx in

    SCR2 = 0;
    // turn it off and back on...
    MSTPCR |= MSTPCR_SCI2;
    MSTPCR &= ~MSTPCR_SCI2;

    SMR2 = smr;
    SCMR2 = scmr;
    BRR2 = brr;
}
