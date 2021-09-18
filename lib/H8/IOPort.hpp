//2019-01-18
#pragma once
#include "Registers.hpp"

enum class IOPort
{
    _1 = 0,
    _2,
    _3,
    _4,
    _5,
    _6,
    A = 9,
    B,
    C,
    D,
    E,
    F,
    G
};

enum class IODir
{
    In = 0,
    Out
};

static uint8_t cachedDDR[16] = {0}; // direction registers are write-only

inline void setPortDirection(IOPort port, int mask, IODir dir)
{
    if(dir == IODir::In)
        cachedDDR[static_cast<int>(port)] &= ~mask;
    else
        cachedDDR[static_cast<int>(port)] |= mask;

    *(&P1DDR + static_cast<int>(port)) = cachedDDR[static_cast<int>(port)];
}

inline void setPinDirection(IOPort port, int bit, IODir dir)
{
    if(dir == IODir::In)
        cachedDDR[static_cast<int>(port)] &= ~PIN_BIT(bit);
    else
        cachedDDR[static_cast<int>(port)] |= PIN_BIT(bit);

    *(&P1DDR + static_cast<int>(port)) = cachedDDR[static_cast<int>(port)];
}

inline uint8_t readPort(IOPort port)
{
    return *(&PORT1 + static_cast<int>(port));
}

inline bool readPin(IOPort port, int bit)
{
    return *(&PORT1 + static_cast<int>(port)) & PIN_BIT(bit);
}

inline void writePort(IOPort port, uint8_t val)
{
    *(&P1DR + static_cast<int>(port)) = val;
}

inline void writePin(IOPort port, int pin, bool val)
{
    if(val)
        *(&P1DR + static_cast<int>(port)) |= PIN_BIT(pin);
    else
        *(&P1DR + static_cast<int>(port)) &= ~PIN_BIT(pin);

}

/*inline void writeConstPin(IOPort port, int pin, bool val)
{
    // only works for constants
    if(val)
    {
        asm volatile(
            "bset %1,%0\n"
            :: "m" (*(&P1DR + static_cast<int>(port))), "i" (pin)
        );
    }
    else
    {
        asm volatile(
            "bclr %1,%0\n"
            :: "m" (*(&P1DR + static_cast<int>(port))), "i" (pin)
        );
    }
}*/

