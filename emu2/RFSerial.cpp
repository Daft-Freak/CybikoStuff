#include <cstring>
#include <iostream>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "RFSerial.h"

#include "CRC.h"
#include "Util.h"

RFSerial::RFSerial()
{
    const char *addr = "::";
    const char *multicastAddr = "FF01::1";
    const int port = 21062; // "RF"

    recvFd = socket(AF_INET6, SOCK_DGRAM, 0);
    sendFd = socket(AF_INET6, SOCK_DGRAM, 0);

    if(recvFd != -1)
    {
        // allow reuse
        int yes = 1;
        setsockopt(recvFd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char *>(&yes), sizeof(int));

        struct sockaddr_in6 sinAddr = {};
        sinAddr.sin6_family = AF_INET6;
        sinAddr.sin6_port = htons(port);

        bool success = inet_pton(AF_INET6, addr, &sinAddr.sin6_addr) == 1;

        success = success && ::bind(recvFd, (struct sockaddr *)&sinAddr, sizeof(sinAddr)) != -1;

        // setup multicast
        struct ipv6_mreq group;
        group.ipv6mr_interface = 0;
        success = success && inet_pton(AF_INET6, multicastAddr, &group.ipv6mr_multiaddr) == 1;
        success = success && setsockopt(recvFd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &group, sizeof(group)) != -1;

        if(success)
            printf("Listening for RF data on %s port %i\n", addr, port);
        else
        {
            close(recvFd);
            recvFd = -1;
        }
    }

    if(sendFd != -1)
    {
        auto sinAddr = new sockaddr_in6{};
        sinAddr->sin6_family = AF_INET6;
        sinAddr->sin6_port = htons(port);

        if(inet_pton(AF_INET6, multicastAddr, &sinAddr->sin6_addr) == 1)
            sendAddr = sinAddr;
        else
        {
            close(sendFd);
            sendFd = -1;
        }
    }
}

RFSerial::~RFSerial()
{
    if(recvFd != -1)
        close(recvFd);

    if(sendFd != -1)
    {
        close(sendFd);
        delete (sockaddr_in6 *)sendAddr;
    }
}

uint8_t RFSerial::read()
{
    auto val = writeQueue.front();
    writeQueue.pop_front();
    return val;
}

void RFSerial::write(uint8_t val)
{
    buf[bufOffset++] = val;

    if(!messageLen)
    {
        switch(val)
        {
            case 0x01: // some kind of command
                messageLen = 3;
                break;

            case 0x30: // short message
                messageLen = 52;
                break;
            
            case 0xCF: // long message
                messageLen = 202;
                break;

            default:
                printf("RFSerial: %02X\n", val);
                bufOffset = 0;
        }
    }
    else if(bufOffset == messageLen)
    {
        if(buf[0] == 0x01 && buf[1] == 2)
        {
            printf("RF channel = %i\n", buf[2]);
        }
        else if(buf[0] == 0x30 || buf[0] == 0xCF)
        {
            // these are mostly the same other than length
            bool longPkt = buf[0] == 0xCF;
            auto packetBuf = buf + 2;

            uint32_t dstAddr = (packetBuf[0] << 24) | (packetBuf[1] << 16) | (packetBuf[2] << 8) | (packetBuf[3]);
            uint32_t srcAddr = (packetBuf[4] << 24) | (packetBuf[5] << 16) | (packetBuf[6] << 8) | (packetBuf[7]);
            int channel = packetBuf[8] & 0x3F; // top bits = 0xC0?
            uint8_t type = packetBuf[9] >> 5; // ping, message, ack, sys
            uint8_t flags = packetBuf[9] & 0x1F; // ?  
            uint8_t index = packetBuf[10]; //?
            uint8_t unk11 = packetBuf[11];
            uint16_t dataCRC = (packetBuf[12] << 8) | packetBuf[13];
            uint16_t headCRC = (packetBuf[14] << 8) | packetBuf[15];
            int headLen = 16;
            int dataLen = longPkt ? 104 : 14;
            int footLen = longPkt ? 80 : 20;

            auto srcAddrStr = cyIDToString(srcAddr);
            auto dstAddrStr = cyIDToString(dstAddr);

            printf("RFSerial packet from %08X(@%s) to %08X(@%s) on chan %i\n", srcAddr, srcAddrStr.c_str(), dstAddr, dstAddrStr.c_str(), channel);
            printf("\ttype %i flags? %x index? %i unk11 %x data crc %04X header CRC %04X\n", type, flags, index, unk11, dataCRC, headCRC);
            printf("\tdata:");

            for(int i = 0; i < dataLen; i++)
            {
                bool newline = i > 0 && i % 16 == 0;
                printf("%s%02X", newline ? "\n\t      " : " ", packetBuf[i + headLen]);
            }
            printf("\n");

            // this is calculated from the data somehow
            printf("\tfoot:");

            for(int i = 0; i < footLen; i++)
            {
                bool newline = i > 0 && i % 16 == 0;
                printf("%s%02X", newline ? "\n\t      " : " ", packetBuf[i + headLen + dataLen]);
            }
            printf("\n");

            // should be a delay here...
            writeQueue.push_back(0x03);

            // forward to network
            if(sendFd != -1)
            {
                size_t bufSize = headLen + dataLen + footLen + 8;
                auto outBuf = new uint8_t[bufSize];
                // extra prefix
                outBuf[0] = outBuf[1] = outBuf[2] = outBuf[3] = outBuf[4] = outBuf[5] = 0xAA;
                outBuf[6] = 0;
                outBuf[7] = longPkt ? 0xC8 : 0x32;
                memcpy(outBuf + 8, packetBuf, headLen + dataLen + footLen);

                sendto(sendFd, outBuf, bufSize, 0, (sockaddr *)sendAddr, sizeof(sockaddr_in6));

                delete[] outBuf;
            }
        }
        else
        {
            printf("RFSerial msg %02X\n", buf[0]);
            printf("\t");
            for(int i = 0; i < messageLen; i++)
                printf(" %02X", buf[i]);

            printf("\n");
        }

        messageLen = 0;
        bufOffset = 0;
    }
}

bool RFSerial::canRead()
{
    return !writeQueue.empty();;
}

void RFSerial::networkUpdate()
{
    if(recvFd == -1)
        return;

    fd_set fds;
    int maxFd = recvFd;
    FD_ZERO(&fds);
    FD_SET(recvFd, &fds);

    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    int ready = select(maxFd + 1, &fds, nullptr, nullptr, &timeout);

    if(ready < 0)
        return; // oh no
    else if(ready == 0)
        return;

    // it's ready
    uint8_t buf[1024];
    sockaddr_storage addr;
    socklen_t addrLen = sizeof(addr);
    int len = recvfrom(recvFd, buf, sizeof(buf), 0, (sockaddr *)&addr, &addrLen);

    if(len > 0)
    {
        // blindly forward to cybiko, what could possibly go wrong?
        for(int i = 0; i < len; i++)
            writeQueue.push_back(buf[i]);
    }
}
