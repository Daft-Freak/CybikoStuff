// split out 20 dec 2019
// probably written in jan

#include <cstring>
#include <iostream>

#include "USBDevice.h"
#include "USBRegisters.h"

static bool usbipGetDescriptor(struct usbip_client *client, uint32_t seqnum, uint8_t descType, uint8_t descIndex, uint16_t setupIndex, uint16_t setupLength, void *userData)
{
    auto usb = reinterpret_cast<USBDevice *>(userData);
    return usb->usbipGetDescriptor(client, seqnum, descType, descIndex, setupIndex, setupLength);
}

static bool usbipControlRequest(struct usbip_client *client, uint32_t seqnum, uint8_t requestType, uint8_t request, uint16_t value, uint16_t index, uint16_t length, const uint8_t *outData, void *userData)
{
    auto usb = reinterpret_cast<USBDevice *>(userData);
    return usb->usbipControlRequest(client, seqnum, requestType, request, value, index, length, outData);
}

static bool usbipIn(struct usbip_client *client, uint32_t seqnum, int ep, uint32_t length, void *userData)
{
    auto usb = reinterpret_cast<USBDevice *>(userData);
    return usb->usbipIn(client, seqnum, ep, length);
}

static bool usbipOut(struct usbip_client *client, uint32_t seqnum, int ep, uint32_t length, const uint8_t *data, void *userData)
{
    auto usb = reinterpret_cast<USBDevice *>(userData);
    return usb->usbipOut(client, seqnum, ep, length, data);
}

static bool usbipUnlink(struct usbip_client *client, uint32_t seqnum, void *userData)
{
    auto usb = reinterpret_cast<USBDevice *>(userData);
    return usb->usbipUnlink(client, seqnum);
}

USBDevice::USBDevice(H8CPU &cpu) : cpu(cpu)
{
    reset();
}

uint8_t USBDevice::read(uint32_t addr)
{
    if(addr & 0x1)
    {
        std::cerr << "Attempt to read USB address register!" << std::endl;
        return 0;
    }

    switch(static_cast<USBReg>(regAddr))
    {
        case USBReg::MAEV:
            return getMAEV();
        case USBReg::MAMSK:
            return mainMask;
        case USBReg::ALTEV:
        {
            auto tmp = altEvent;
            altEvent &= ~(ALTEV_EOP | ALTEV_SD3 | ALTEV_SD5 | ALTEV_RESET | ALTEV_RESUME);
            updateInterrupt();
            return tmp;
        }
        case USBReg::ALTMSK:
            return altMask;
        case USBReg::TXEV:
            return txEvent;
        case USBReg::TXMSK:
            return txMask;
        case USBReg::RXEV:
            return rxEvent;
        case USBReg::RXMSK:
            return rxMask;
        case USBReg::NAKEV:
        {
            auto tmp = nakEvent;
            nakEvent = 0; // cleared on read
            updateInterrupt();
            return tmp;
        }
        case USBReg::NAKMSK:
            return nakMask;

        case USBReg::RXD0:
        {
            uint8_t v = 0;
            if(controlFIFO.getFilled())
                v = controlFIFO.pop();

            return v;
        }

        // test hax
        case USBReg::TXS0:
        {
            // ack?
            bool done = (txEvent & TXEV_FIFO0);
            txEvent &= ~TXEV_FIFO0;
            updateInterrupt();
            return (done ? TXS0_DONE | TXS0_ACK_STAT : 0) | (8 - controlFIFO.getFilled());
        }

        case USBReg::RXS0:
            // reading status clears RXEV
            rxEvent &= ~RXEV_FIFO0;
            updateInterrupt();

            if(controlFIFO.getFilled() == 8)
                return RXS0_SETUP | 8;
            else
                return controlFIFO.getFilled();
        //
    }

    std::cout << "USB r " << std::hex << static_cast<int>(regAddr) << "(" << getRegName(regAddr) << ")" << std::dec << std::endl;

    return 0;
}

void USBDevice::write(uint32_t addr, uint8_t val)
{
    //handle address write
    if(addr & 0x1)
    {
        regAddr = val;
        return;
    }

    switch(static_cast<USBReg>(regAddr))
    {
        case USBReg::MCNTRL:
            if(val & MCNTRL_SRST)
                reset();

            if(val & 0xFE)
                std::cout << "USB w MCNTRL = " << std::hex << static_cast<int>(val) << std::dec << std::endl;
            break;

        case USBReg::MAMSK:
            mainMask = val;
            break;

        case USBReg::ALTMSK:
            altMask = val;
            break;

        case USBReg::TXMSK:
            txMask = val;
            break;

        case USBReg::RXMSK:
            rxMask = val;
            break;

        case USBReg::NAKMSK:
            nakMask = val;
            break;

        case USBReg::TXC0:
            txEnable[0] = (val & TXC0_EN) != 0;

            // TODO: handle unsent data if sending...
            if(val & TXC0_FLUSH)
                controlFIFO.reset();

            if(val & TXC0_IGN_IN)
                std::cout << "USB TXC0 IGN_IN\n";

            if(txEnable[0])
            {
                if(controlFIFO.empty())
                {
                    nakEvent |= NAKEV_OUT0;
                    updateInterrupt();

                    if(usbipServer && usbipInSeqnum[0])
                    {
                        // should be end of data if we have any
                        if(usbipInDataOffset[0] || !usbipInDataLen[0])
                            usbip_client_reply(usbipLastClient, usbipInSeqnum[0], usbipInData[0], usbipInDataLen[0]);
                        else
                        {
                            printf("usbip stall in %i\n", usbipInSeqnum[0]);
                            usbip_client_stall(usbipLastClient, usbipInSeqnum[0]);
                        }
                        usbipInSeqnum[0] = 0;
                    }
                }
                else if(enumerationState != EnumerationState::Done)
                    updateEnumeration();
                else if(usbipServer && usbipInSeqnum[0])
                {
                    // thanks to our tiny FIFO, we need to buffer here
                    while(!controlFIFO.empty())
                        usbipInData[0][usbipInDataOffset[0]++] = controlFIFO.pop();

                    if(usbipInDataOffset[0] == usbipInDataLen[0])
                    {
                        usbip_client_reply(usbipLastClient, usbipInSeqnum[0], usbipInData[0], usbipInDataLen[0]);
                        usbipInSeqnum[0] = 0;
                    }
                }

                if(controlFIFO.empty())
                {
                    txEvent |= TXEV_FIFO0;
                    updateInterrupt();
                }
            }
            break;

        case USBReg::TXD0:
            if(controlFIFO.getFilled() < 8)
                controlFIFO.push(val);
            break;

        case USBReg::RXC0:
            rxEnable[0] = (val & RXC0_EN) != 0;

            if(val & RXC0_FLUSH)
                controlFIFO.reset();

            if(val & RXC0_IGN_OUT)
                std::cout << "USB RXC0 IGN_OUT\n";
            if(val & RXC0_IGN_SETUP)
                std::cout << "USB RXC0 IGN_OUT\n";

            if(rxEnable[0])
                updateEnumeration();

            break;
        default:
            std::cout << "USB w " << std::hex << static_cast<int>(regAddr) << "(" << getRegName(regAddr) << ") = " << static_cast<int>(val) << std::dec << std::endl;
    }
}

void USBDevice::startEnumeration()
{
    if(enumerationState != EnumerationState::NotStarted)
        return;

    enumerationState = EnumerationState::RequestDeviceDesc;

    updateEnumeration();
}

void USBDevice::usbipUpdate()
{
    if(!usbipServer)
        return;

    timeval timeout;

    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    usbip_server_update(usbipServer, &timeout);
}

bool USBDevice::usbipGetDescriptor(usbip_client *client, uint32_t seqnum, uint8_t descType, uint8_t descIndex, uint16_t setupIndex, uint16_t setupLength)
{
    if(rxEnable[0] && controlFIFO.empty())
    {
        if(usbipInData[0])
            delete[] usbipInData[0];

        // re-assemble setup packet
        controlFIFO.push(0x80);
        controlFIFO.push(0x6);
        controlFIFO.push(descIndex);
        controlFIFO.push(descType);
        controlFIFO.push(setupIndex);
        controlFIFO.push(setupIndex >> 8);
        controlFIFO.push(setupLength);
        controlFIFO.push(setupLength >> 8);

        usbipLastClient = client;
        usbipInSeqnum[0] = seqnum;

        usbipInData[0] = new uint8_t[setupLength];

        usbipInDataLen[0] = setupLength;
        usbipInDataOffset[0] = 0;

        rxEvent |= RXEV_FIFO0;
        updateInterrupt();

        return true;
    }
    return false;
}

bool USBDevice::usbipControlRequest(usbip_client *client, uint32_t seqnum, uint8_t requestType, uint8_t request, uint16_t value, uint16_t index, uint16_t length, const uint8_t *outData)
{
    if(outData) // copy out data
    {
        if(usbipOutData[0])
            delete[] usbipOutData[0];

        usbipOutData[0] = new uint8_t[length];
        usbipOutDataLen[0] = length;
        usbipOutDataOffset[0] = 0;
        memcpy(usbipOutData[0], outData, length);
    }

    bool in = (requestType & 0x80) || !length; // 0-length always in?

    if(!rxEnable[0] && !controlFIFO.empty())
        return false;

    // re-assemble setup packet
    controlFIFO.push(requestType);
    controlFIFO.push(request);
    controlFIFO.push(value);
    controlFIFO.push(value >> 8);
    controlFIFO.push(index);
    controlFIFO.push(index >> 8);
    controlFIFO.push(length);
    controlFIFO.push(length >> 8);

    usbipLastClient = client;

    if(in)
    {
        if(length)
        {
            if(usbipInData[0])
                delete[] usbipInData[0];

            usbipInData[0] = new uint8_t[length];
        }

        usbipInDataLen[0] = length;
        usbipInDataOffset[0] = 0;
        usbipInSeqnum[0] = seqnum;
    }
    else
        usbipOutSeqnum[0] = seqnum;

    rxEvent |= RXEV_FIFO0;
    updateInterrupt();

    return true;
}

bool USBDevice::usbipIn(usbip_client *client, uint32_t seqnum, int ep, uint32_t length)
{
    printf("usbip in %i %i %i\n", seqnum, ep, length);
    return false;
}

bool USBDevice::usbipOut(usbip_client *client, uint32_t seqnum, int ep, uint32_t length, const uint8_t *data)
{
    printf("usbip out %i %i %i\n", seqnum, ep, length);
    return false;
}

bool USBDevice::usbipUnlink(usbip_client *client, uint32_t seqnum)
{
    for(auto &n : usbipInSeqnum)
    {
        if(n == seqnum)
        {
            n = 0;
            return true;
        }
    }

    for(auto &n : usbipOutSeqnum)
    {
        if(n == seqnum)
        {
            n = 0;
            return true;
        }
    }

    return true; // should be more strict here, but we're losing seqnums sometimes
}

void USBDevice::updateEnumeration()
{
    auto setupPacket = [this](uint8_t requestType, uint8_t request, uint16_t value, uint16_t index, uint16_t length)
    {
        controlFIFO.push(requestType);
        controlFIFO.push(request);
        controlFIFO.push(value);
        controlFIFO.push(value >> 8);
        controlFIFO.push(index);
        controlFIFO.push(index >> 8);
        controlFIFO.push(length);
        controlFIFO.push(length >> 8);

        rxEvent |= RXEV_FIFO0;
        updateInterrupt();
    };

    auto canSendPacket = [this]
    {
        // there's only one fifo, so having tx and rx events set is going to confuse things
        return rxEnable[0] && controlFIFO.empty() && !(txEvent & TXEV_FIFO0);
    };

    switch(enumerationState)
    {
        case EnumerationState::NotStarted:
        case EnumerationState::Done:
            break;

        case EnumerationState::RequestDeviceDesc:
        {
            if(!canSendPacket())
                return;

            setupPacket(0x80, 6 /*GET_DESCRIPTOR*/, 1 << 8 /*device*/, 0, 18);

            enumerationState = EnumerationState::ReadDeviceDesc;
            deviceDescOffset = 0;
            break;
        }
        
        case EnumerationState::ReadDeviceDesc:
        {
            if(!txEnable[0])
                return;

            while(!controlFIFO.empty() && deviceDescOffset < 18)
                deviceDesc[deviceDescOffset++] = controlFIFO.pop();

            if(deviceDescOffset == 18)
            {
                printf("USB device descriptor:\n");

                printf("\tbLength            %i\n", deviceDesc[0]);
                printf("\tbDescriptorType    %i\n", deviceDesc[1]);
                printf("\tbcdUSB             %i.%02i\n", deviceDesc[3], deviceDesc[2]);
                printf("\tbDeviceClass       %i\n", deviceDesc[4]);
                printf("\tbDeviceSubClass    %i\n", deviceDesc[5]);
                printf("\tbDeviceProtocol    %i\n", deviceDesc[6]);
                printf("\tbMaxPacketSize     %i\n", deviceDesc[7]);
                printf("\tidVendor           %04X\n", deviceDesc[8] | (deviceDesc[9] << 8));
                printf("\tidProduct          %04X\n", deviceDesc[10] | (deviceDesc[11] << 8));
                printf("\tbcdDevice          %i.%02i\n", deviceDesc[13], deviceDesc[12]);
                printf("\tiManufacturer      %i\n", deviceDesc[14]);
                printf("\tiProduct           %i\n", deviceDesc[15]);
                printf("\tiSerialNumber      %i\n", deviceDesc[16]);
                printf("\tbNumConfigurations %i\n", deviceDesc[17]);

                //altEvent |= ALTEV_RESET; // reset?

                enumerationState = EnumerationState::SetAddress;
            }
            break;
        }
        
        case EnumerationState::SetAddress: // reset done
        {
            if(!canSendPacket())
                return;

            setupPacket(0, 5 /*SET_ADDRESS*/, 1 /*addr*/, 0, 0);

            enumerationState = EnumerationState::RequestConfigDesc;
            break;
        }

        case EnumerationState::RequestConfigDesc: // set addr done
        {
            if(!canSendPacket())
                return;

            // get config descriptor
            setupPacket(0x80, 6 /*GET_DESCRIPTOR*/, 2 << 8 /*configuration*/, 0, 9);

            configDescOffset = 0;
            enumerationState = EnumerationState::RequestFullConfigDesc;
            break;
        }

        case EnumerationState::RequestFullConfigDesc:
        {
            if(!txEnable[0] && configDescOffset < 9)
                return;

            uint8_t buf[8];

            if(configDescOffset < 8)
            {
                // we just want the len, which should be at the start
                while(!controlFIFO.empty())
                    buf[configDescOffset++] = controlFIFO.pop();

                configDescLen = buf[2] | buf[3] << 8;
                printf("USB config desc len %i\n", configDescLen);
            }
            else if(configDescOffset < 9)
            {
                // discard the rest (which is one byte...)
                while(!controlFIFO.empty())
                {
                    controlFIFO.pop();
                    configDescOffset++;
                }
            }
            else
            {
                if(!canSendPacket())
                    return;

                // now get the whole thing
                setupPacket(0x80, 6 /*GET_DESCRIPTOR*/, 2 << 8 /*configuration*/, 0, configDescLen);

                enumerationState = EnumerationState::ReadConfigDesc;

                if(configDesc)
                    delete[] configDesc;

                configDesc = new uint8_t[configDescLen];
                configDescOffset = 0;
            }
            break;
        }

        case EnumerationState::ReadConfigDesc:
        {
            if(!txEnable[0])
                return;

            while(!controlFIFO.empty())
                configDesc[configDescOffset++] = controlFIFO.pop();

            if(configDescOffset == configDescLen)
            {
                printf("USB configuration descriptor:\n");

                printf("\tbLength             %i\n", configDesc[0]);
                printf("\tbDescriptorType     %i\n", configDesc[1]);
                printf("\twTotalLength        %04X\n", configDesc[2] | configDesc[3] << 8);
                printf("\tbNumInterfaces      %i\n", configDesc[4]);
                printf("\tbConfigurationValue %i\n", configDesc[5]);
                printf("\tiConfiguration      %i\n", configDesc[6]);
                printf("\tbmAttributes        %02X\n", configDesc[7]);
                printf("\tbMaxPower           %i\n", configDesc[8]);

                // interfaces
                auto desc = configDesc + 9;
                auto end = desc + configDescOffset;
                for(int i = 0; i < configDesc[4] && desc < end;)
                {
                    if(!desc[0])
                        break;

                    if(desc[1] == 4)
                    {
                        printf("\tinterface descriptor:\n");
                        printf("\t\tbLength            %i\n", desc[0]);
                        printf("\t\tbDescriptorType    %i\n", desc[1]);
                        printf("\t\tbInterfaceNumber   %i\n", desc[2]);
                        printf("\t\tbAlternateSetting  %i\n", desc[3]);
                        printf("\t\tbNumEndpoints      %i\n", desc[4]);
                        printf("\t\tbInterfaceClass    %i\n", desc[5]);
                        printf("\t\tbInterfaceSubClass %i\n", desc[6]);
                        printf("\t\tbInterfaceProtocol %i\n", desc[7]);
                        printf("\t\tiInterface         %i\n", desc[8]);

                        int numEP = desc[4];

                        i++;
                        desc += desc[0];
                        
                        // endpoints
                        for(int ep = 0; ep < numEP && desc < end;)
                        {
                            if(!desc[0])
                                break;

                            if(desc[1] == 5)
                            {
                                printf("\t\tendpoint descriptor:\n");
                                printf("\t\t\tbLength          %i\n", desc[0]);
                                printf("\t\t\tbDescriptorType  %i\n", desc[1]);
                                printf("\t\t\tbEndpointAddress %02X\n", desc[2]);
                                printf("\t\t\tbmAttributes     %i\n", desc[3]);
                                printf("\t\t\twMaxPacketSize   %04X\n", desc[4] | desc[5] << 8);
                                printf("\t\t\tbInterval        %i\n", desc[6]);
                                ep++;
                            }
                            else
                                printf("\t\tdesc %i len %i\n", desc[1], desc[0]);

                            desc += desc[0];
                        }
                    }
                    else
                    {
                        printf("\tdesc %i len %i\n", desc[1], desc[0]);
                        desc += desc[0];
                    }
                }

                // switch over to usbip now that we have the descriptors
                usbip_remove_device(&usbipDev);

                usbipDev.device_descriptor = deviceDesc;
                usbipDev.config_descriptor = configDesc;
                usbipDev.speed = usbip_speed_full;
                usbipDev.user_data = this;
                usbipDev.get_descriptor = ::usbipGetDescriptor;
                usbipDev.control_request = ::usbipControlRequest;
                usbipDev.in = ::usbipIn;
                usbipDev.out = ::usbipOut;
                usbipDev.unlink = ::usbipUnlink;

                usbip_add_device(&usbipDev);
            
                enumerationState = EnumerationState::Done;
            }

            break;
        }
    }
}

void USBDevice::updateInterrupt()
{
    if(getMAEV() & mainMask)
        cpu.externalInterrupt(1);
}

void USBDevice::reset()
{
    mainMask = 0;
    altMask = 0;
    txMask = 0;
    rxMask = 0;
    nakMask = 0;

    altEvent = 0;
    txEvent = 0;
    rxEvent = 0;
    nakEvent = 0;

    for(int i = 0; i < 4; i++)
        txEnable[i] = rxEnable[i] = false;

    controlFIFO.reset();

    // enumeration
    bool enumEnabled = enumerationState != EnumerationState::NotStarted;
    enumerationState = EnumerationState::NotStarted;
    configDesc = nullptr;
    configDescLen = 0;
    configDescOffset = 0;

    if(enumEnabled)
        startEnumeration();

    // setup USBIP if enumeration enabled
    if(enumEnabled)
    {
        if(usbipServer)
        {
            usbip_destroy_server(usbipServer);
            usbipServer = nullptr;
        }

        if(usbip_create_server(&usbipServer, "::1", 0) != usbip_success)
            std::cerr << "USBIP server create failed!\n";
    
        for(auto &num : usbipInSeqnum)
            num = 0;
        for(auto &num : usbipOutSeqnum)
            num = 0;

        for(auto &buf : usbipInData)
            buf = nullptr;
        for(auto &len : usbipInDataLen)
            len = 0;
        for(auto &off : usbipInDataOffset)
            off = 0;

        for(auto &buf : usbipOutData)
            buf = nullptr;
        for(auto &len : usbipOutDataLen)
            len = 0;
        for(auto &off : usbipOutDataOffset)
            off = 0;
    }
}

uint8_t USBDevice::getMAEV() const
{
    uint8_t v = 0;

    if(altEvent & altMask)
        v |= MAEV_ALT;
    
    if(txEvent & txMask)
        v |= MAEV_TX;

    if(rxEvent & rxMask)
        v |= MAEV_RX;

    if(nakEvent & nakMask)
        v |= MAEV_NAK;

    return v;
}

const char *USBDevice::getRegName(int reg)
{
    const char *regNames[]
    {
        "MCNTRL",
        "CCONF",
        "", // 0x2 reserved
        "RID",
        "FAR",
        "NFSR",
        "MAEV",
        "MAMSK",
        "ALTEV",
        "ALTMSK",
        "TXEV",
        "TXMSK",
        "RXEV",
        "RXMSK",
        "NAKEV",
        "NAKMSK",
        "FWEV",
        "FWMSK",
        "FNH",
        "FNL",
        "DMACNTRL",
        "DMAEV",
        "DMAMSK",
        "MIR",
        "DMACNT",
        "DMAERR",
        "", // 0x1A reserved
        "WKUP",
        "", // 0x1C - 0x1F reserved
        "",
        "",
        "",
        "EPC0",
        "TXD0",
        "TXS0",
        "TXC0",
        "", // 0x24 reserved
        "RXD0",
        "RXS0",
        "RXC0",
        "EPC1",
        "TXD1",
        "TXS1",
        "TXC1",
        "EPC2",
        "RXD1",
        "RXS1",
        "RXC1",
        "EPC3",
        "TXD2",
        "TXS2",
        "TXC2",
        "EPC4",
        "RXD2",
        "RXS2",
        "RXC2",
        "EPC5",
        "TXD3",
        "TXS3",
        "TXC3",
        "EPC6",
        "RXD3",
        "RXS3",
        "RXC3"
    };

    if(reg >= 64)
        return "??";

    return regNames[reg];
}
