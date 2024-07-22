#include <cstring>
#include <fstream>
#include <iostream>

#include "SerialFlash.h"

uint8_t SerialFlash::read()
{
    didWrite = false;
    if(outputStatus)
        //return 0xA8; // ready (bit 7) | density (bits 5-3 / 101) // larger flash (528 byte blocks)
        return 0x98; // ready (bit 7) density (bits 5-3 / 011)

    // need to start responses the byte after reading the command, not on the last byte of the command
    if(wait)
    {
        wait = false;
        return 0xFF;
    }

    if(readAddr != 0xFFFFFFFF)
    {
        //std::cout << "Flash read " << readAddr + readOff << "\n";
        auto val = mem[readAddr + readOff];
        readOff = (readOff + 1) % pageSize;

        return val;
    }

    return 0xFF;
}

void SerialFlash::write(uint8_t val)
{
    // active low
    if(cs)
        return;

    didWrite = true;

    if(outputStatus || readAddr != 0xFFFFFFFF) return;

    if(writeAddr != 0xFFFFFFFF)
    {
        writeBuf[writeOff++] = val;
        if(writeOff >= pageSize)
            writeOff = 0;
        return;
    }

    if(len != 0)
    {
        // read command
        buf[off++] = val;

        if(off == len)
        {
            if(opCode == 0x52)
            {
                int byteAddr = buf[2] | ((buf[1] & 1) << 8);
                int pageAddr = (buf[1] >> 1) | (buf[0] << 7);
                // std::cout << "Flash read page " << pageAddr << " + " << byteAddr << "\n";
                readAddr = pageAddr * pageSize + byteAddr;
                readOff = 0;
                wait = true;
            }
            else if(opCode == 0x82)
            {
                int byteAddr = buf[2] | ((buf[1] & 1) << 8);
                int pageAddr = (buf[1] >> 1) | (buf[0] << 7);
                // std::cout << "Flash write page " << pageAddr << " + " << byteAddr << "\n";
                writeAddr = pageAddr * pageSize + byteAddr;
                writeOff = 0;
            }

            len = 0;
            opCode = 0;
        }
    }
    else if(opCode == 0)
    {
        opCode = val;
        off = 0;

        switch(val)
        {
            case 0x52: // main memory read
                len = 7;
                break;

            case 0x57: // status
                opCode = 0; // no args
                outputStatus = true;
                break;

            case 0x60: // compare buffer to main memory (ignore, bit is fixed at 0 for success)
                len = 3;
                break;

            case 0x82: // main memory program through buffer
                len = 3;
                break;

            default:
                std::cout << std::hex << "Flash op " << static_cast<int>(val) << "\n" << std::dec;
                opCode = 0;
        }
    }
}

bool SerialFlash::canRead() 
{
    // TODO: CS
    return didWrite;
}

void SerialFlash::setCS(bool val)
{
    // end operation
    if(val && !cs)
    {
        outputStatus = false;
        readAddr = 0xFFFFFFFF;

        // do writes
        if(writeAddr != 0xFFFFFFFF)
        {
            //std::cout << "flash prog\n";

            int off = writeAddr % pageSize; // use offset?
            writeAddr -= off;
            memcpy(mem + writeAddr, writeBuf, pageSize);

            writeAddr = 0xFFFFFFFF;
        }
    }

    cs = val;
}

bool SerialFlash::loadFile(const char *filename, int offset)
{
    std::ifstream file(filename);
    if(!file)
        return false;

    file.read(reinterpret_cast<char *>(mem + offset), 0x84000 - offset);

    std::cout << "Read " << file.gcount() << " bytes into serial flash" << std::endl;

    return true;
}

void SerialFlash::saveFile(const char *filename, int offset, int length)
{
    std::ofstream file(filename);
    if(!file)
        return;

    if(length == -1)
        length = 0x84000 - offset;

    file.write(reinterpret_cast<char *>(mem + offset), length);
}
