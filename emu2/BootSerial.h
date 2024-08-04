#ifndef BOOTSERIAL_H
#define BOOTSERIAL_H

#include <deque>
#include <fstream>
#include <sstream>

#include "H8CPU.h"

class SDCard final
{
public:
    uint8_t read();

    void write(uint8_t val);

    bool canRead();

    void setFile(std::string filename);

private:
    void sendBlock();

    enum Status
    {
        Status_Idle          = 1 << 0,
        Status_AppCmd        = 1 << 1,
        Status_Read_Multi    = 1 << 2,
        Status_Writing       = 1 << 3,
        Status_WriteGotToken = 1 << 4,
    };

    bool didWrite = false;

    int status = 0;

    // block read/write
    int readAddress = 0, writeAddress = 0;

    uint8_t writeBuf[512];
    int writeOffset = 0;

    // cmd parsing
    uint8_t cmd[6];
    int cmdOff = 0;

    // output
    std::deque<uint8_t> readQueue;

    std::fstream file;
};

// used for logging and commands at boot
// main serial port / Serial2 on classic
// cart port / Serial1 on xtreme (used for SD card later)
class BootSerial final : public SerialDevice
{
public:
    BootSerial(int index);

    uint8_t read() override;

    void write(uint8_t val) override;

    bool canRead() override;

    void setBootFile(std::string filename);

    SDCard &getSD() {return sd;}

private:
    uint16_t crc(uint8_t *data, int length);

    int index;

    // log port
    std::stringstream logBuffer;

    // port also connected to SD card
    bool sdMode;
    SDCard sd;

    // for booting a file
    bool gotPrepareMsg = false;

    uint8_t bootBuf[512 + 9];
    int bootBufLen = 0, bootBufOff = 0;

    std::ifstream bootFile;
};

#endif