#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>

#include <SDL.h>

#include "DS2401.h"
#include "H8CPU.h"
#include "KeyboardDevice.h"
#include "LCDDevice.h"
#include "MemoryDevice.h"
#include "PCF8593.h"
#include "USBDevice.h"

// from usbcon.c
static uint32_t crc32Table[256] = {0};

void initCRCTables()
{
    for(int i = 0; i < 256; i++)
    {
        uint32_t crc = i;
        for(int j = 0; j < 8; j++)
        {
            bool bit = crc & 1;
            crc >>= 1;

            if(bit)
                crc ^= 0xEDB88320;
        }

        crc32Table[i] = crc;
    }
}

uint32_t crc32(uint8_t *data, int length)
{
    uint32_t crc = 0xFFFFFFFF;

    for(int i = 0; i < length; i++)
    {
        uint8_t v = (crc & 0xFF) ^ data[i];
        crc = (crc >> 8) ^ crc32Table[v];
    }

    return crc;
}

//


// dec 2019
class PortA : public IODevice
{
protected:
    uint8_t read() override
    {
        // red led high (| 0x40) is on charge
        // On/Off high (pullup)
        return 0x80 | 0x40;
    }

    void write(uint8_t val) override
    {

    }

    void setDirection(uint8_t dir) override
    {

    }
};


// log port at boot, cart port later (connected to SD card)
// can also be used to boot things
class Serial1 : public SerialDevice
{
public:
    Serial1()
    {
    }

    uint8_t read() override
    {
        if(gotPrepareMsg && bootBufOff < bootBufLen)
            return bootBuf[bootBufOff++];

        return 0xFF;
    }

    void write(uint8_t val) override
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

                std::cout << "SERIAL1: " << logBuffer.str() << std::endl;
                logBuffer.str("");

            }
            else if(val == 0xFF) // probably doing SPI now
            {
                std::cout << "Switching serial 1 to SD card...\n";
                sdMode = true;
            }
            return;
        }
    }

    bool canRead() override
    {
        if(gotPrepareMsg && bootBufOff < bootBufLen)
            return true;

        return false;
    }

    void setBootFile(std::string filename)
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
private:
    uint16_t crc(uint8_t *data, int length)
    {
        uint32_t crc = 0;

        for(int i = 0; i < length; i++)
            crc = (crc << 1) ^ (data[i] << 1);

        crc = (crc & 0xFFFF) ^ (crc >> 17);

        return crc;
    }

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

int main(int argc, char *args[])
{
    static const uint64_t clockFreq = 18432000;

    initCRCTables();

    if(SDL_Init(SDL_INIT_TIMER) < 0)
        return 1;

    // get data path
    std::string dataPath = "../data/";

    auto basePath = SDL_GetBasePath();

    if(basePath)
    {
        dataPath = basePath + dataPath;
        SDL_free(basePath);
    }
    std::cout << "Data path: " << dataPath << "\n";

    // options
    const bool xtreme = true; // TODO maybe?
    bool persistExtRAM = true;
    std::string serialBootFile;

    bool benchmarkMode = false;
    int64_t benchmarkCycles = 0;

    for(int i = 1; i < argc; i++)
    {
        std::string arg(args[i]);

        if(arg == "--boot")
            serialBootFile = args[++i];
        else if(arg == "--benchmark-time")
        {
            benchmarkMode = true;
            benchmarkCycles = clockFreq * std::stoi(args[++i]);
        }
    }

    // CPU init
    H8CPU cpu;

    auto lcd = std::make_unique<LCDDevice>();
    auto usb = std::make_unique<USBDevice>();
    auto extRAM = std::make_unique<MemoryDevice>();
    std::unique_ptr<MemoryDevice> flash;
    std::unique_ptr<KeyboardDevice> keyboard;

    std::unique_ptr<PCF8593> rtc;
    std::unique_ptr<DS2401> serialNo;
    std::unique_ptr<IODevice> miscPortA;

    std::unique_ptr<SerialDevice> serial1;

    if(xtreme)
    {
        flash = std::make_unique<MemoryDevice>(0x7FFFF);
        keyboard = std::make_unique<XtremeKeyboardDevice>();

        rtc = std::make_unique<PCF8593>(6, 1);
        serialNo = std::make_unique<DS2401>(cpu);
        miscPortA = std::make_unique<PortA>();

        cpu.setExternalArea(0, lcd.get()); //lcd
        cpu.setExternalArea(1, usb.get()); //usb
        cpu.setExternalArea(2, extRAM.get()); //external ram
        cpu.setExternalArea(3, flash.get()); //flash
        cpu.setExternalArea(7, keyboard.get()); //keyboard matrix

        cpu.addIODevice(IOPort::_6, serialNo.get());
        cpu.addIODevice(IOPort::A, miscPortA.get());
        cpu.addIODevice(IOPort::F, rtc.get());

        serial1 = std::make_unique<Serial1>();
        cpu.setSerialDevice(1, serial1.get());

        if(!cpu.loadROM((dataPath + "xtreme-rom.bin").c_str()))
        {
            std::cerr << "Failed to load ROM!\n";
            return 1;
        }
        flash->loadFile((dataPath + "xtreme-flash.bin").c_str());

        // ram persistence for FS
        if(persistExtRAM)
            extRAM->loadFile((dataPath + "exram.bin").c_str());

        // can still boot over serial
        if(!serialBootFile.empty())
            static_cast<Serial1 *>(serial1.get())->setBootFile(serialBootFile);
    }

    // change id for fun
    if(xtreme)
    {
        flash->write(0x7F818, 0x14);
        flash->write(0x7F819, 0x08);
        flash->write(0x7F81A, 0x20);
        flash->write(0x7F81B, 0xA9);
        uint32_t crc = ~crc32(flash->getData() + 0x7F800, 0x7FC);
        flash->write(0x7FFFC, crc >> 24);
        flash->write(0x7FFFD, crc >> 16);
        flash->write(0x7FFFE, crc >> 8);
        flash->write(0x7FFFF, crc);
    }


    cpu.reset();

    //rendering setup

    SDL_Window *sdlWindow;
    SDL_Renderer *sdlRenderer;
    sdlWindow = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1600, 1000, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_PRESENTVSYNC);
    SDL_RenderSetLogicalSize(sdlRenderer, 160, 100);

    auto texture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 160, 100);
    uint8_t screenBuffer[160 * 100 * 4]{0};

    bool running = true;
    bool runCPU = true;
    auto lastTime = SDL_GetPerformanceCounter();

    while(running)
    {
        rtc->updateTime();

        auto nowTime = SDL_GetPerformanceCounter();
        auto cpuCycles = ((nowTime - lastTime) * clockFreq) / SDL_GetPerformanceFrequency();

        //add handled time
        lastTime += (cpuCycles * SDL_GetPerformanceFrequency()) / clockFreq;

        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            switch(event.type)
            {
                case SDL_KEYDOWN:
                case SDL_KEYUP:
                    keyboard->updateKey(event.key.keysym, event.key.state == SDL_PRESSED);
                    if(xtreme)
                        cpu.externalInterrupt(5);
                    break;
                case SDL_QUIT:
                    running = false;
                    break;
            }
        }

        if(cpuCycles > clockFreq / 30)
        {
            std::cout << "Running a bit slow (last update " << ((cpuCycles * 1000) / clockFreq) << " ms)\n";
            if(cpu.getSleeping())
                std::cout << "\t... currently sleeping...\n";

            cpuCycles /= 2;
        }

        //execute
        if(runCPU)
        {
            if(!cpu.executeCycles(cpuCycles))
            {
                cpu.dumpRegs();
                runCPU = false;
                break;
            }

            if(benchmarkMode)
            {
                benchmarkCycles -= cpuCycles;
                if(benchmarkCycles < 0)
                    running = false;
            }
        }

        usb->update(cpu);

        //update screen
        // TODO: sync
        lcd->convertDisplay(screenBuffer);
        SDL_UpdateTexture(texture, nullptr, screenBuffer, 160 * 4);
        SDL_RenderCopy(sdlRenderer, texture, nullptr, nullptr);
        SDL_RenderPresent(sdlRenderer);
    }

    if(xtreme && persistExtRAM && !benchmarkMode)
        extRAM->saveFile((dataPath + "exram.bin").c_str());

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(sdlRenderer);
    SDL_DestroyWindow(sdlWindow);

    return 0;
}
