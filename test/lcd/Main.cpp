#include "LCDDriver.h"

static LCDDriver lcd;

int main()
{
    // write some initial data
    auto fb = lcd.getFramebuffer();

    for(int y = 0; y < lcd.getHeight(); y++)
    {
        for(int x = 0; x < lcd.getStride(); x++)
            fb[x + y * lcd.getStride()] = (y / 25) * 0b01010101;
    }

    lcd.setDirty();
    lcd.updateDisplay();

    while(true)
    {
    }
    return 0;
}
