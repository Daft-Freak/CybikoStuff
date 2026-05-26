#ifndef LCDDRIVER_H
#define LCDDRIVER_H

#include <stdint.h>

class LCDDriver final
{
public:
    LCDDriver();

    void setDirty(uint8_t minY = 0, uint8_t maxY = 0xFF);
    void updateDisplay();

    int getWidth() const {return width;}
    int getHeight() const {return height;}
    int getBitsPerPixel() const {return bitsPerPixel;}
    int getStride() const {return stride;}

    uint8_t *getFramebuffer() {return framebuffer;}

private:
    static const int width = 160;
    static const int height = 100;
    static const int bitsPerPixel = 2;
    static const int stride = width / (8 / bitsPerPixel);
    uint8_t framebuffer[stride * height];

    uint8_t minDirtyY, maxDirtyY;
};

#endif