#include "KeyboardDevice.h"

// 20 dec 2019
XtremeKeyboardDevice::XtremeKeyboardDevice()
{
    for(auto &v : keyData)
        v = 0xFFFF;
}

uint8_t XtremeKeyboardDevice::read(uint32_t addr)
{
    uint8_t ret = 0;

    bool low = addr & 1;

    // address bits 1-10 are the columns
    for(int i = 1; i < 11; i++)
    {
        if(!(addr & (1 << i)))
            ret |= (keyData[i - 1] >> (low ? 0 : 8));
    }

    return ret;
}

void XtremeKeyboardDevice::write(uint32_t addr, uint8_t val)
{
}

void XtremeKeyboardDevice::updateKey(SDL_Keysym sym, bool down)
{
    static const int columns[]
    {
        -1, -1, -1, -1,  3,  1,  2,  2, // A-D
         3,  2,  1,  1,  0,  1,  0,  0, // E-L
         0,  1,  0,  9,  3,  2,  3,  2, // M-T
         1,  2,  3,  3,  1,  3, -1, -1, // U-Z 1-2
        -1, -1, -1, -1, -1, -1, -1, -1, // 3-0
         4,  5, -1,  5,  4, -1, -1,  2, // Enter, Esc, Backspace, Tab, Space, -, =, [
         0, -1, -1,  9, -1, -1,  0,  9, // ], \, ??, ;, ', `, comma, .
        -1, -1,  6,  5,  4,  3,  2,  1, // /, caps, F1-6
         0, -1, -1, -1, -1, -1, -1, -1, // F7-12, printscreen, scroll lk
        -1,  5, -1, -1,  5, -1, -1,  6, // pause, ins, home, pgup, del, end, pgdn, right
         6,  6,  6,// left, down, up
    };

    static const int rows[]
    {
        -1, -1, -1, -1,  2,  2,  9,  8, // A-D
         7, 12,  1,  4,  2,  7, 11, 14, // E-L
         8,  3, 13,  4,  1, 13,  5, 14, // M-T
         6, 11,  6,  4,  5,  3, -1, -1, // U-Z 1-2
        -1, -1, -1, -1, -1, -1, -1, -1, // 3-0
         3, 10, -1,  7,  6, -1, -1, 10,
         9, -1, -1,  3, -1, -1, 10,  1,
        -1, -1,  0,  0,  0,  0,  0,  0,
         0, -1, -1, -1, -1, -1, -1, -1,
        -1,  9, -1, -1,  8, -1, -1, 12,
        14, 13, 11,
    };

    // select, !, ?
    // [] are mapped to ()
    // [ is Fn+( on the cybiko keyboard...

    int col = -1, row = -1;

    if(sym.scancode <= SDL_SCANCODE_UP)
    {
        col = columns[sym.scancode];
        row = rows[sym.scancode];
    }
    // map ctrl -> fn
    else if(sym.scancode == SDL_SCANCODE_LCTRL || sym.scancode == SDL_SCANCODE_RCTRL)
    {
        col = 7;
        row = 15;
    }
    else if(sym.scancode == SDL_SCANCODE_LSHIFT || sym.scancode == SDL_SCANCODE_RSHIFT)
    {
        col = 8;
        row = 15;
    }
    // menu
    else if(sym.scancode == SDL_SCANCODE_APPLICATION)
    {
        col = 4;
        row = 5;
    }

    if(col != -1)
    {
        if(down)
            keyData[col] &= ~(1 << row);
        else
            keyData[col] |= (1 << row);
    }
}
