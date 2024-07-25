#ifndef USBDEVICE_H
#define USBDEVICE_H

#include "H8CPU.h"


class USBDevice final : public ExternalAddressDevice
{
public:
    USBDevice(H8CPU &cpu);

    uint8_t read(uint32_t addr) override;
    void write(uint32_t addr, uint8_t val) override;

private:
    template<int size>
    class FIFO
    {
    public:
        void reset()
        {
            readOff = writeOff = 0;
            full = false;
        }

        uint8_t pop()
        {
            auto ret = buf[readOff++];
            readOff %= size;
            full = false;
            return ret;
        }

        void push(uint8_t v)
        {
            buf[writeOff++] = v;
            writeOff %= size;

            if(readOff == writeOff)
                full = true;
        }

        int getFilled()
        {
            if(full)
                return size;

            if(writeOff >= readOff)
                return writeOff - readOff;
            else
                return (writeOff + size) - readOff;
        }

    private:
        uint8_t buf[size];
        int writeOff = 0, readOff = 0;
        bool full = false;
    };


    void reset();

    uint8_t getMAEV() const;

    const char *getRegName(int reg);

    H8CPU &cpu;

    uint8_t regAddr;

    // event masks
    uint8_t mainMask = 0, altMask = 0, txMask = 0, rxMask = 0, nakMask = 0;

    // event status
    uint8_t altEvent = 0, txEvent = 0, rxEvent = 0, nakEvent = 0;

    // enables
    bool txEnable[4], rxEnable[4];

    // fifo
    FIFO<8> controlFIFO;
};

#endif
