//2019-12-11
#pragma once

#include "Registers.hpp"

void sci0Init(uint8_t smr, uint8_t scmr, uint8_t brr);
void sci1Init(uint8_t smr, uint8_t scmr, uint8_t brr);
void sci2Init(uint8_t smr, uint8_t scmr, uint8_t brr);

template<int i>
void sciWrite(uint8_t v)
{
    // wait for tx empty
    while(!(SSRx(i) & SSR_TDRE)) {}

    // set data
    TDRx(i) = v;
    SSRx(i) &= ~SSR_TDRE;

    // wait
    //while(!(SSRx(i) & SSR_TEND)) {}
    while(!(SSRx(i) & SSR_TDRE)) {}
}

template<int i>
void sciWrite(const uint8_t *v, int len)
{
    for(int j = 0; j < len; j++)
        sciWrite<i>(*(v++));
}

template<int i>
uint8_t sciRead()
{
    // TODO: handle orer
    if(SSRx(i) & SSR_ORER)
        SSRx(i) &= ~SSR_ORER;

    // wait
    while(!(SSRx(i) & SSR_RDRF)) {}

    uint8_t ret = RDRx(i);
    // clear rx data full
    SSRx(i) &= ~SSR_RDRF;

    return ret;
}

template<int i>
void sciRead(uint8_t *v, int len)
{
    for(int j = 0; j < len; j++)
        *(v++) = sciRead<i>();
}

template<int i>
uint8_t sciTransfer(uint8_t v)
{
    // wait for tx empty
    while(!(SSRx(i) & SSR_TDRE)) {}

    // set data
    TDRx(i) = v;
    SSRx(i) &= ~SSR_TDRE;

    // handle orer?

    // wait
    while(!(SSRx(i) & SSR_RDRF)) {}

    uint8_t ret = RDRx(i);
    // clear rx data full
    SSRx(i) &= ~SSR_RDRF;

    return ret;
}

template<int i>
void sciTransfer(const uint8_t *out, uint8_t *in, int len)
{
    for(int j = 0; j < len; j++)
        in[j] = sciTransfer<i>(out[j]);
}


// TODO: DMA read/write/transfer? (only for 0/1)

#define sci0Read sciRead<0>
#define sci1Read sciRead<1>
#define sci2Read sciRead<2>

#define sci0Write sciWrite<0>
#define sci1Write sciWrite<1>
#define sci2Write sciWrite<2>

#define sci0Transfer sciTransfer<0>
#define sci1Transfer sciTransfer<1>
#define sci2Transfer sciTransfer<2>
