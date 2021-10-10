// split out 20 dec 2019
// probably written in jan

#include <fstream>
#include <iostream>

#include "MemoryDevice.h"

MemoryDevice::MemoryDevice(uint32_t addressMask, bool debug) : debug(debug), addressMask(addressMask){}

uint8_t MemoryDevice::read(uint32_t addr)
{
    addr &= addressMask;

    if(debug)
        std::cout << "external read " << std::hex << addr << " (= " << static_cast<int>(mem[addr]) << ")" << std::dec << std::endl;
    return mem[addr];
}

void MemoryDevice::write(uint32_t addr, uint8_t val)
{
    addr &= addressMask;

    if(debug)
        std::cout << "external write " << std::hex << addr << " = " << static_cast<int>(val) << std::dec << std::endl;

    mem[addr] = val;
}

uint8_t *MemoryDevice::getPtr(uint32_t &mask)
{
    if(debug)
        return nullptr;

    mask = addressMask;
    return mem;
}

bool MemoryDevice::loadFile(const char *filename, int offset)
{
    std::ifstream file(filename);
    if(!file)
        return false;

    file.read(reinterpret_cast<char *>(mem + offset), 0x200000 - offset);

    std::cout << "Read " << file.gcount() << " bytes into external memory area" << std::endl;

    return true;
}

void MemoryDevice::saveFile(const char *filename, int offset, int length)
{
    std::ofstream file(filename);
    if(!file)
        return;

    if(length == -1)
        length = 0x200000 - offset;

    file.write(reinterpret_cast<char *>(mem + offset), length);
}
