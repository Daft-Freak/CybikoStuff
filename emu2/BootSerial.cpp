#include <iostream>

#include "BootSerial.h"

uint32_t crc32(uint8_t *, int);

BootSerial::BootSerial(int index) : index(index)
{
}

uint8_t BootSerial::read()
{
    if(gotPrepareMsg && bootBufOff < bootBufLen)
        return bootBuf[bootBufOff++];

    return 0xFF;
}

void BootSerial::write(uint8_t val)
{
    if(!sdMode)
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