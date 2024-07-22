#ifndef KEYBOARDDEVICE_H
#define KEYBOARDDEVICE_H

#include "SDL_keyboard.h"

#include "H8CPU.h"

class KeyboardDevice : public ExternalAddressDevice
{
public:
    virtual void updateKey(SDL_Keysym sym, bool down) = 0;
};

// classic keyboard has more keys, xtreme keyboard has less conflicts

// 21 july 2020
class ClassicKeyboardDevice : public KeyboardDevice
{
public:
    ClassicKeyboardDevice();

    uint8_t read(uint32_t addr) override;
    void write(uint32_t addr, uint8_t val) override;

    void updateKey(SDL_Keysym sym, bool down) override;

protected:
    uint8_t keyData[9];
};


// 20 dec 2019
class XtremeKeyboardDevice final : public KeyboardDevice
{
public:
    XtremeKeyboardDevice();

    uint8_t read(uint32_t addr) override;
    void write(uint32_t addr, uint8_t val) override;

    void updateKey(SDL_Keysym sym, bool down) override;

protected:
    uint16_t keyData[10];
};


#endif
