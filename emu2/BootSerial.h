#ifndef BOOTSERIAL_H
#define BOOTSERIAL_H

#include <fstream>
#include <sstream>

#include "H8CPU.h"

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

private:
    uint16_t crc(uint8_t *data, int length);

    int index;

    // log port
    std::stringstream logBuffer;

    // port also connected to SD card
    bool sdMode;

    // for booting a file
    bool gotPrepareMsg = false;

    uint8_t bootBuf[512 + 9];
    int bootBufLen = 0, bootBufOff = 0;

    std::ifstream bootFile;
};

#endif