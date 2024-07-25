#ifndef USBDEVICE_H
#define USBDEVICE_H

#include "usbip.h"

#include "H8CPU.h"


class USBDevice final : public ExternalAddressDevice
{
public:
    USBDevice(H8CPU &cpu);

    uint8_t read(uint32_t addr) override;
    void write(uint32_t addr, uint8_t val) override;

    void startEnumeration();

    void usbipUpdate();

    bool usbipGetDescriptor(struct usbip_client *client, uint32_t seqnum, uint8_t descType, uint8_t descIndex, uint16_t setupIndex, uint16_t setupLength);
    bool usbipControlRequest(struct usbip_client *client, uint32_t seqnum, uint8_t requestType, uint8_t request, uint16_t value, uint16_t index, uint16_t length, const uint8_t *outData);
    bool usbipIn(struct usbip_client *client, uint32_t seqnum, int ep, uint32_t length);
    bool usbipOut(struct usbip_client *client, uint32_t seqnum, int ep, uint32_t length, const uint8_t *data);
    bool usbipUnlink(struct usbip_client *client, uint32_t seqnum);

private:
    enum class EnumerationState
    {
        NotStarted = 0,
        RequestDeviceDesc,
        ReadDeviceDesc, // and reset
        SetAddress,
        RequestConfigDesc,
        RequestFullConfigDesc,
        ReadConfigDesc, // and init usbip
        Done
    };

    template<int size>
    class FIFO
    {
    public:
        void reset()
        {
            readOff = writeOff = 0;
            fullFlag = false;
        }

        uint8_t pop()
        {
            auto ret = buf[readOff++];
            readOff %= size;
            fullFlag = false;
            return ret;
        }

        void push(uint8_t v)
        {
            buf[writeOff++] = v;
            writeOff %= size;

            if(readOff == writeOff)
                fullFlag = true;
        }

        int getFilled()
        {
            if(fullFlag)
                return size;

            if(writeOff >= readOff)
                return writeOff - readOff;
            else
                return (writeOff + size) - readOff;
        }

        bool empty() const
        {
            return !fullFlag && readOff == writeOff;
        }

        bool full() const
        {
            return fullFlag;
        }

    private:
        uint8_t buf[size];
        int writeOff = 0, readOff = 0;
        bool fullFlag = false;
    };


    void reset();

    void updateEnumeration();
    void updateInterrupt();

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

    // enumeration state
    EnumerationState enumerationState;
    uint8_t deviceDesc[18];
    int deviceDescOffset;
    uint8_t *configDesc = nullptr;
    int configDescLen, configDescOffset;

    // usbip
    usbip_server *usbipServer = nullptr;
    usbip_device usbipDev;

    usbip_client *usbipLastClient;

    uint32_t usbipInSeqnum[16];
    uint32_t usbipOutSeqnum[16];

    uint8_t *usbipInData[16];
    uint32_t usbipInDataLen[16];
    uint32_t usbipInDataOffset[16];

    uint8_t *usbipOutData[16];
    uint32_t usbipOutDataLen[16];
    uint32_t usbipOutDataOffset[16];
};

#endif
