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

    eccInit();
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
            int headLen = 16;
            int dataLen = longPkt ? 104 : 14;

            dumpPacket("", packetBuf, longPkt);

            // should be a delay here...
            writeQueue.push_back(0x03);

            // forward to network
            if(sendFd != -1)
            {
                size_t bufSize = headLen + dataLen + 8;
                auto outBuf = new uint8_t[bufSize];
                // extra prefix
                outBuf[0] = outBuf[1] = outBuf[2] = outBuf[3] = outBuf[4] = outBuf[5] = 0xAA;
                outBuf[6] = 0;
                outBuf[7] = longPkt ? 0xC8 : 0x32;
                memcpy(outBuf + 8, packetBuf, headLen + dataLen);

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
        bool longPkt = buf[7] == 0xC8;

        // fill in the ECC data
        eccCalc(buf + 8, longPkt);
        len += longPkt ? 80 : 20;

        dumpPacket(" (from socket)", buf + 8, longPkt);

        // blindly forward to cybiko, what could possibly go wrong?
        for(int i = 0; i < len; i++)
            writeQueue.push_back(buf[i]);
    }
}

void RFSerial::dumpPacket(const char *suffix, uint8_t *packetBuf, bool longPkt)
{
    uint32_t dstAddr = (packetBuf[0] << 24) | (packetBuf[1] << 16) | (packetBuf[2] << 8) | (packetBuf[3]);
    uint32_t srcAddr = (packetBuf[4] << 24) | (packetBuf[5] << 16) | (packetBuf[6] << 8) | (packetBuf[7]);
    int channel = packetBuf[8] & 0x3F; // top bits = 0xC0?
    uint8_t type = packetBuf[9] >> 5; // ping, message, ack, sys
    uint8_t flags = packetBuf[9] & 0x1F; // ?  
    uint8_t index = packetBuf[10]; // 0 = single packet, something else for pings?
    uint8_t sequence = packetBuf[11]; // 0 = no ack
    uint16_t dataCRC = (packetBuf[12] << 8) | packetBuf[13];
    uint16_t headCRC = (packetBuf[14] << 8) | packetBuf[15];
    int headLen = 16;
    int dataLen = longPkt ? 104 : 14;
    int eccLen = longPkt ? 80 : 20;

    auto srcAddrStr = cyIDToString(srcAddr);
    auto dstAddrStr = cyIDToString(dstAddr);

    printf("RFSerial packet from %08X(@%s) to %08X(@%s) on chan %i%s\n", srcAddr, srcAddrStr.c_str(), dstAddr, dstAddrStr.c_str(), channel, suffix);
    
    if(type == 0) // ping
    {
        auto data = packetBuf + headLen;
        printf("\tping, flags %X head unk %X %X data unk %02X %02X %02X %02X\n", flags, index, sequence, data[0], data[1], data[2], data[3]);
        
        int nameLen = strnlen(reinterpret_cast<char *>(data + 4), 8);
        int age = data[12] & 0x7F;
        bool gender = data[12] & 0x80;
        
        printf("\tname %.*s age %i gender: %s\n", nameLen, data + 4, age, gender ? "female" : "male");
    }
    else if(type == 1) // message
    {
        auto data = packetBuf + headLen;
        uint16_t flags = data[0] << 8 | data[1];
        uint32_t param0 = 0, param1 = 0;
        uint8_t bufLen = 0;
        int offset = 2;

        printf("\tmessage, flags %04X index %i seq %i", flags, index, sequence);

        if(index <= 1)
        {
            // id
            uint16_t msgId = data[2] << 8 | data[3];
            offset += 2;

            printf(" id %04X\n\t", msgId);

            // params
            if(flags & (1 << 11))
            {
                param0 = data[offset + 0] << 24 | data[offset + 1] << 16 | data[offset + 2] << 8 | data[offset + 3];
                offset += 4;
                printf("param0 %08X ", param0);
            }

            if(flags & (1 << 10))
            {
                param1 = data[offset + 0] << 24 | data[offset + 1] << 16 | data[offset + 2] << 8 | data[offset + 3];
                offset += 4;
                printf("param1 %08X ", param1);
            }

            if(flags & (1 << 13))
            {
                // TODO: untested
                auto name = reinterpret_cast<char *>(data) + offset;
                // app name
                int len = strnlen(reinterpret_cast<char *>(data) + offset, dataLen - offset);
                offset += len;
                printf("app name %s\n", name);
            }
            else
            {
                // app name is 16-bit index
                // TODO: 2 is finder, 37 is chat?
                uint16_t appName = data[offset + 0] << 8 | data[offset + 1];
                offset += 2;
                printf("app %04X ", appName);
            }

            if(flags & (1 << 12))
            {
                // if it's an incomplete buffer, the length is the rest of the packet
                if(flags & (1 << 9))
                    bufLen = dataLen - offset;
                else
                    bufLen = data[offset++];

                printf("buf size %u ", bufLen);
            }
        }
        else if(flags & (1 << 8))
        {
            // final
            bufLen = data[offset++];
            printf(" buf size %u", bufLen);
        }
        else if(flags & (1 << 9))
        {
            // more data
            bufLen = dataLen - 2;
            printf(" buf size %u", bufLen);
        }

        printf("\n");

        // dump buffer
        if(bufLen)
        {
            printf("\tbuf:");
            for(int i = 0; i < bufLen; i++)
            {
                bool newline = i > 0 && i % 16 == 0;
                printf("%s%02X", newline ? "\n\t     " : " ", data[i + offset]);
            }
            printf("\n");
        }
    }
    else
    {
        printf("\ttype %i flags? %x index? %i unk11 %x data crc %04X header CRC %04X\n", type, flags, index, unk11, dataCRC, headCRC);
        printf("\tdata:");

        for(int i = 0; i < dataLen; i++)
        {
            bool newline = i > 0 && i % 16 == 0;
            printf("%s%02X", newline ? "\n\t      " : " ", packetBuf[i + headLen]);
        }
        printf("\n");

        // reed-solomon error correction
        printf("\tecc: ");

        for(int i = 0; i < eccLen; i++)
        {
            bool newline = i > 0 && i % 16 == 0;
            printf("%s%02X", newline ? "\n\t      " : " ", packetBuf[i + headLen + dataLen]);
        }
        printf("\n");
    }
}

void RFSerial::eccInit()
{
    // calculate the tables

    int e = 1;
    for(int i = 0; i < 255; i++)
    {
        rsExpTable[i] = e;
        rsLogTable[e] = i;

        e <<= 1;

        if(e >= 256)
            e = (e ^ 0x1D) & 0xFF;
    }

    rsExpTable[255] = 0;
    rsLogTable[0] = 255;

    auto genPoly = [this](int numRoots, uint8_t *out)
    {
        out[0] = 0;

        for(int i = 0; i < numRoots; i++)
        {
            int root = i + 1;

            out[i + 1] = 1;

            for(int j = i; j > 0; j--)
            {
                if(out[j] != 0)
                    out[j] = out[j - 1] ^ rsExpTable[rsMod(rsLogTable[out[j]] + root)];
                else
                    out[j] = out[j - 1];
            }

            out[0] = rsExpTable[rsMod(rsLogTable[out[0]] + root)];
        }

        for(int i = 0; i <= numRoots; i++)
            out[i] = rsLogTable[out[i]];
    };

    genPoly(20, rsPoly20);
    genPoly(80, rsPoly80);
}

void RFSerial::eccCalc(uint8_t *data, bool longPkt)
{
    int dataLen = longPkt ? 120 : 30;
    int eccLen = longPkt ? 80 : 20;
    auto poly = longPkt ? rsPoly80 : rsPoly20;

    auto ecc = data + dataLen;
    memset(ecc, 0, eccLen);

    for(int i = dataLen - 1; i >= 0; i--)
    {
        uint8_t b = rsLogTable[ecc[eccLen - 1] ^ data[i]];

        if(b == 0xFF)
        {
            for(int j = eccLen - 1; j > 0; j--)
                ecc[j] = ecc[j - 1];

            ecc[0] = 0;
        }
        else
        {
            for(int j = eccLen - 1; j > 0; j--)
                ecc[j] = rsExpTable[rsMod(b + poly[j])] ^ ecc[j - 1];

            ecc[0] = rsExpTable[rsMod(b + poly[0])];
        }
    }
}

// modulo for reed-solomon
uint8_t RFSerial::rsMod(unsigned v)
{
    while(v >= 0xFF)
    {
        v -= 0xFF;
        v = (v & 0xFF) + ((v >> 8) & 0xFF);
    }

    return v;
}
