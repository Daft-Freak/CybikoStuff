#include <filesystem>
#include <iostream>

#include "BootSerial.h"

#include "CRC.h"

uint8_t SDCard::read()
{
    if(readQueue.empty())
        return 0xFF;

    auto ret = readQueue.front();
    readQueue.pop_front();

    return ret;
}

void SDCard::write(uint8_t val)
{
    didWrite = true;

    if(cmdOff != 0)
    {
        cmd[cmdOff++] = val;

        // continue reading command
        if(cmdOff == 6)
        {
            std::cout << "Got SD " << ((status & Status_AppCmd) ? "a" : "") << "cmd " << (cmd[0] & 0x3F) << std::hex;
            for(int i = 1; i < 6; i++)
                std::cout << " " << static_cast<int>(cmd[i]);
            std::cout << "\n" << std::dec;

            readQueue.push_back(0xFF); // data sent while reading the last byte

            if(status & Status_AppCmd)
            {
                status &= ~Status_AppCmd;
                switch(cmd[0] & 0x3F)
                {
                    case 41: // send op cond
                        status &= ~Status_Idle;
                        readQueue.push_back(0x00);
                        break;
                }
            }
            else
            {
                uint32_t arg = (cmd[1] << 24) | (cmd[2] << 16) | (cmd[3] << 8) | cmd[4];
                switch(cmd[0] & 0x3F)
                {
                    case 0: // go idle
                        status |= Status_Idle;
                        readQueue.push_back(0x01);
                        break;

                    case 9: // send CSD
                    {
                        readQueue.push_back((status & Status_Idle) ? 0x01 : 0x00); // res
                        //readQueue.push_back(0xFF); // pretend we need some time
                        readQueue.push_back(0xFE); // start token

                        // copied from 64MB card
                        uint8_t csd[16]{0x00, 0x36, 0x00, 0x32, 0x17, 0x59, 0x81, 0xD7, 0xF6, 0xDA, 0x7F, 0x81, 0x96, 0x40, 0x00, 0x51};

                        for(auto b : csd)
                            readQueue.push_back(b);

                        readQueue.push_back(0xFF); // crc
                        readQueue.push_back(0xFF); // driver doesn't care
                        break;
                    }

                    case 10: // send CID
                    {
                        readQueue.push_back((status & Status_Idle) ? 0x01 : 0x00); // res
                        readQueue.push_back(0xFE); // start token

                        // very placeholder-y (OEM ID = "MM", product name = "AAAAA")
                        uint8_t cid[16]{0x00, 0x4D, 0x4D, 0x41, 0x41, 0x41, 0x41, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF1};

                        for(auto b : cid)
                            readQueue.push_back(b);

                        readQueue.push_back(0xFF); // crc
                        readQueue.push_back(0xFF);
                        break;
                    }

                    case 12: // stop transmission
                    {
                        status &= ~Status_Read_Multi;
                        readQueue.clear();

                        readQueue.push_back(0xFF); // stuff byte
                        readQueue.push_back(0x00);
                        break;
                    }

                    case 17: // read block
                        readQueue.push_back(0x00);

                        readAddress = arg;

                        sendBlock();
                        break;

                    case 18: // read multiple blocks
                        readQueue.push_back(0x00);

                        readAddress = arg;
                        status |= Status_Read_Multi;

                        sendBlock();
                        break;

                    case 24: // write block
                        readQueue.push_back(0x00);
                        writeAddress = arg;
                        status |= Status_Writing;
                        status &= ~Status_WriteGotToken;
                        writeOffset = 0;
                        break;

                    case 55: // app cmd
                        status |= Status_AppCmd;
                        readQueue.push_back((status & Status_Idle) ? 0x01 : 0x00);
                        break;

                    default:
                        // send 0x4 | idle (illegal command)
                        std::cout << "Unhandled SD CMD" << static_cast<int>(cmd[0] & 0x3F) << "\n";
                        readQueue.push_back((status & Status_Idle) ? 0x05 : 0x04);
                        break;
                }
            }

            cmdOff = 0;
        }
    }
    // block writes
    else if(status & Status_Writing)
    {
        if(!(status & Status_WriteGotToken) && val == 0xFE)
            status |= Status_WriteGotToken;
        else if(status & Status_WriteGotToken)
        {
            // got data + crc
            if(writeOffset == 512 + 1)
            {
                //readQueue.push_back(0xFF); // while writing this byte (not needed as RX is disabled here?..)
                readQueue.push_back(0x05); // data accepted
                readQueue.push_back(0x00); // busy
                status &= ~Status_Writing;

                file.clear();
                file.seekp(writeAddress);
                file.write(reinterpret_cast<char *>(writeBuf), 512);
            }
            else 
            {

                if(writeOffset < 512)
                    writeBuf[writeOffset] = val;
                // keep going for 2 bytes to ignore the CRC
                writeOffset++;
            }
        }
    }
    else if((val & 0xC0) == 0x40) // start command if valid
        cmd[cmdOff++] = val;

    // continue reading blocks
    if((status & Status_Read_Multi) && readQueue.empty())
        sendBlock();
}

bool SDCard::canRead()
{
    // TODO: CS
    bool ret = didWrite;
    didWrite = false;
    return ret;
}

void SDCard::setFile(std::string filename)
{
    if(std::filesystem::exists(filename))
        file.open(filename, std::ios::in | std::ios::out | std::ios::binary);
    else // create
        file.open(filename, std::ios::in | std::ios::out | std::ios::trunc | std::ios::binary);
}

void SDCard::sendBlock()
{
    readQueue.push_back(0xFF); // pretend we do work

    readQueue.push_back(0xFE); // start token

    uint8_t buf[512];
    file.clear();
    file.seekg(readAddress);
    file.read(reinterpret_cast<char *>(buf), 512);

    for(auto b : buf)
        readQueue.push_back(b);

    readQueue.push_back(0xFF); // crc
    readQueue.push_back(0xFF); // 

    readAddress += 512;
}

BootSerial::BootSerial(int index) : index(index)
{
}

uint8_t BootSerial::read()
{
    if(sdMode)
        return sd.read();

    if(gotPrepareMsg && bootBufOff < bootBufLen)
        return bootBuf[bootBufOff++];

    return 0xFF;
}

void BootSerial::write(uint8_t val)
{
    if(sdMode)
        sd.write(val);
    else
    {
        // buffer/print (has boot messages)
        logBuffer << val;

        if(val == '\n')
        {
            // send boot file
            if(logBuffer.str() == "Preparing to load CyOS\r\n")
                gotPrepareMsg = true;
            else if(logBuffer.str().compare(0, 4, "send") == 0 && bootFile.is_open())
            {
                // send file
                int chunkIndex = std::stoi(logBuffer.str().substr(4));
                const int chunkSize = 512;

                bootFile.clear(); // clear eof
                bootFile.seekg(chunkIndex * chunkSize);
                bootFile.read(reinterpret_cast<char *>(bootBuf + 5), chunkSize);

                auto read = bootFile.gcount();

                std::cout << "send boot chunk " << chunkIndex << " " << read << "/" << chunkSize << "\n";

                bootBuf[0] = 'C';
                bootBuf[1] = chunkIndex & 0xFF;
                bootBuf[2] = chunkIndex >> 8;
                bootBuf[3] = read & 0xFF;
                bootBuf[4] = read >> 8;

                uint32_t crc = ~crc32(bootBuf + 1, read + 4);

                bootBuf[read + 5] = crc & 0xFF;
                bootBuf[read + 6] = (crc >> 8) & 0xFF;
                bootBuf[read + 7] = (crc >> 16) & 0xFF;
                bootBuf[read + 8] = crc >> 24;

                bootBufOff = 0;
                bootBufLen = read + 9;
            }

            std::cout << "SERIAL" << index << ": " << logBuffer.str() << std::endl;
            logBuffer.str("");

        }
        else if(val == 0xFF && index == 1) // probably doing SPI now
        {
            std::cout << "Switching serial 1 to SD card...\n";
            sdMode = true;
        }
        return;
    }
}

bool BootSerial::canRead()
{
    if(sdMode)
        return sd.canRead();

    if(gotPrepareMsg && bootBufOff < bootBufLen)
        return true;

    return false;
}

void BootSerial::setBootFile(std::string filename)
{
    bootFile.open(filename, std::ios::binary);

    bootFile.seekg(0, std::ios::end);
    int fileSize = bootFile.tellg();
    bootFile.seekg(0);

    std::cout << "Booting " << filename << " (" << fileSize << " bytes)\n";

    bootBufLen = snprintf(reinterpret_cast<char *>(bootBuf), sizeof(bootBuf), "rcv file.boot %i\n", fileSize);

    uint16_t crcVal = crc(reinterpret_cast<uint8_t *>(bootBuf), bootBufLen - 1);

    // doesn't seem to matter...
    bootBuf[bootBufLen++] = crcVal & 0xFF;
    bootBuf[bootBufLen++] = crcVal >> 8;
}

uint16_t BootSerial::crc(uint8_t *data, int length)
{
    uint32_t crc = 0;

    for(int i = 0; i < length; i++)
        crc = (crc << 1) ^ (data[i] << 1);

    crc = (crc & 0xFFFF) ^ (crc >> 17);

    return crc;
}