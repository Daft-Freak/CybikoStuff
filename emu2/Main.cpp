#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>

#include <SDL.h>

#include "CRC.h"
#include "BootSerial.h"
#include "DS2401.h"
#include "H8CPU.h"
#include "KeyboardDevice.h"
#include "LCDDevice.h"
#include "MemoryDevice.h"
#include "PCF8593.h"
#include "RFSerial.h"
#include "SerialFlash.h"
#include "USBDevice.h"
#include "Util.h"

class Port1Sound final : public IODevice
{
public:
    uint8_t read(uint32_t time) override
    {
        return 0;
    }

    void write(uint8_t val, uint32_t time) override
    {
        bool newSoundValue = val & (1 << 5);

        if(newSoundValue != lastSoundValue)
        {
            updateAudio(time);
            lastSoundValue = newSoundValue;
        }
    }

    void setDirection(uint8_t dir, uint32_t time) override
    {
    }

    void setAudioDevice(SDL_AudioDeviceID devId)
    {
        audioDev = devId;
    }

    void updateAudio(uint32_t time)
    {
        const unsigned int step = 18432000 / 48000;

        audioCounter += time - lastWriteTime;

        while(audioCounter >= step)
        {
            audioCounter -= step;
            // output value
            int v = lastSoundValue ? 127 : -128;
            audioBuf[bufOffset++] = v;

            if(bufOffset == sizeof(audioBuf))
            {
                if(audioDev)
                    SDL_QueueAudio(audioDev, audioBuf, sizeof(audioBuf));

                bufOffset = 0;
            }
        }

        lastWriteTime = time;
    }

private:
    uint32_t lastWriteTime = 0;
    bool lastSoundValue = false;

    uint32_t audioCounter = 0;

    SDL_AudioDeviceID audioDev = 0;
    int8_t audioBuf[512];
    unsigned int bufOffset = 0;
};

// dec 2019
class PortA : public IODevice
{
protected:
    uint8_t read(uint32_t time) override
    {
        // red led high (| 0x40) is on charge
        // On/Off high (pullup)
        return 0x80 | 0x40;
    }

    void write(uint8_t val, uint32_t time) override
    {

    }

    void setDirection(uint8_t dir, uint32_t time) override
    {

    }
};

class Port1Classic final : public IODevice
{
public:
    uint8_t read(uint32_t time) override
    {
        // otherwise it immediately goes into standby mode
        return 1 << 3;
    }

    void write(uint8_t val, uint32_t time) override
    {
    }

    void setDirection(uint8_t dir, uint32_t time) override
    {
    }
};

// flash cs is on 4
class Port3Classic final : public IODevice
{
public:
    Port3Classic(SerialFlash &flash) : flash(flash) {}

    uint8_t read(uint32_t time) override
    {
        return 0x00;
    }

    void write(uint8_t val, uint32_t time) override
    {
        flash.setCS(val & 0x10);
    }

    void setDirection(uint8_t dir, uint32_t time) override
    {
    }

private:
    SerialFlash &flash;
};

class PortFClassic final : public IODevice
{
protected:
    uint8_t read(uint32_t time) override
    {
        // flash R/B
        return 0x04;
    }

    void write(uint8_t val, uint32_t time) override
    {
    }

    void setDirection(uint8_t dir, uint32_t time) override
    {
    }
};

static void setXtremeBatteryLevel(H8CPU &cpu, std::unique_ptr<MemoryDevice> &extRAM)
{
    // reasonable values for charging, maybe
    cpu.setADCValue(1, 0x32);  // Ua1
    cpu.setADCValue(2, 0x3B0); // Ua2

    // charger status is stored three times before the filesystem
    // filesystem uses 258 byte blocks, the first 2 are a checksum
    // these header blocks only have 250 bytes of data instead of 256
    // the data we want is in a 150 byte block (with a crc32) 100 bytes inside the filesystem block

    // we're going to modify the header to say the battery was last full right now

    uint32_t batLevel = 0x2673C0;
    uint32_t timestamp = cybikoTime();
    
    for(int i = 2; i < 5; i++)
    {
        int blockOff = i * 258;
        int dataOff = blockOff + 115;

        extRAM->write(dataOff +  0, timestamp >> 24);
        extRAM->write(dataOff +  1, timestamp >> 16);
        extRAM->write(dataOff +  2, timestamp >> 8);
        extRAM->write(dataOff +  3, timestamp);

        extRAM->write(dataOff +  4, batLevel >> 24);
        extRAM->write(dataOff +  5, batLevel >> 16);
        extRAM->write(dataOff +  6, batLevel >> 8);
        extRAM->write(dataOff +  7, batLevel);

        extRAM->write(dataOff +  8, timestamp >> 24);
        extRAM->write(dataOff +  9, timestamp >> 16);
        extRAM->write(dataOff + 10, timestamp >> 8);
        extRAM->write(dataOff + 11, timestamp);

        uint32_t crc = ~crc32(extRAM->getData() + blockOff + 102, 146);
        extRAM->write(blockOff + 248, crc >> 24);
        extRAM->write(blockOff + 249, crc >> 16);
        extRAM->write(blockOff + 250, crc >> 8);
        extRAM->write(blockOff + 251, crc);

        uint16_t fsCheck = fsChecksum(extRAM->getData() + blockOff + 2, 250);
        extRAM->write(blockOff + 0, fsCheck >> 8);
        extRAM->write(blockOff + 1, fsCheck);
    }
}

int main(int argc, char *args[])
{
    static const uint64_t clockFreq = 18432000;

    if(SDL_Init(SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO) < 0)
        return 1;

    // get data path
    std::string dataPath = "data/", deviceDataPath;

    auto basePath = SDL_GetBasePath();

    if(basePath)
    {
        dataPath = basePath + dataPath;
        SDL_free(basePath);
    }
    std::cout << "Data path: " << dataPath << "\n";

    // options
    bool xtreme = true;
    bool persistExtRAM = true;
    std::string serialBootFile;
    std::string cyIDStr = "FAKECYB";
    bool usbip = false;
    bool mp3SD = false;

    bool benchmarkMode = false;
    int64_t benchmarkCycles = 0;

    for(int i = 1; i < argc; i++)
    {
        std::string arg(args[i]);

        if(arg == "--boot" && i + 1 < argc)
            serialBootFile = args[++i];
        else if(arg == "--benchmark-time" && i + 1 < argc)
        {
            benchmarkMode = true;
            benchmarkCycles = clockFreq * std::stoi(args[++i]);
        }
        else if(arg == "--cyid" && i + 1 < argc)
        {
            cyIDStr = args[++i];
            if(cyIDStr.length() != 7)
            {
                std::cerr << "CyID should be 7 characters!\n";
                cyIDStr.resize(7, 'A');
            }
        }
        else if(arg == "--classic")
            xtreme = false;
        else if(arg == "--usbip")
            usbip = true;
        else if(arg == "--mp3-sd")
            mp3SD = true; // this is only the SD-card part of the MP3 player
    }

    // CPU init
    H8CPU cpu;

    auto lcd = std::make_unique<LCDDevice>();
    auto extRAM = std::make_unique<MemoryDevice>();
    std::unique_ptr<MemoryDevice> flash;
    std::unique_ptr<KeyboardDevice> keyboard;

    std::unique_ptr<PCF8593> rtc;
    std::unique_ptr<IODevice> port1;

    std::unique_ptr<BootSerial> bootSerial;
    auto rfSerial = std::make_unique<RFSerial>(); //cpu?
    
    // xtreme
    auto usb = std::make_unique<USBDevice>(cpu);
    std::unique_ptr<DS2401> serialNo;
    std::unique_ptr<IODevice> miscPortA;

    // classic
    std::unique_ptr<SerialFlash> serialFlash;
    std::unique_ptr<IODevice> classicPort3, classicPortF;

    if(xtreme)
    {
        flash = std::make_unique<MemoryDevice>(0x7FFFF);
        keyboard = std::make_unique<XtremeKeyboardDevice>();

        port1 = std::make_unique<Port1Sound>();

        rtc = std::make_unique<PCF8593>(6, 1);
        serialNo = std::make_unique<DS2401>(cpu);
        miscPortA = std::make_unique<PortA>();

        cpu.setExternalArea(0, lcd.get()); //lcd
        cpu.setExternalArea(1, usb.get()); //usb
        cpu.setExternalArea(2, extRAM.get()); //external ram
        cpu.setExternalArea(3, flash.get()); //flash
        cpu.setExternalArea(7, keyboard.get()); //keyboard matrix

        cpu.addIODevice(IOPort::_1, port1.get());
        cpu.addIODevice(IOPort::_6, serialNo.get());
        cpu.addIODevice(IOPort::A, miscPortA.get());
        cpu.addIODevice(IOPort::F, rtc.get());

        bootSerial = std::make_unique<BootSerial>(1);
        cpu.setSerialDevice(1, bootSerial.get());
        cpu.setSerialDevice(2, rfSerial.get());

        // load rom/flash/ram dumps
        deviceDataPath = dataPath + cyIDStr + "/";

        if(!std::filesystem::exists(deviceDataPath))
            std::filesystem::create_directory(deviceDataPath);

        if(!cpu.loadROM((dataPath + "xtreme-rom.bin").c_str()))
        {
            std::cerr << "Failed to load ROM!\n";
            return 1;
        }
        flash->loadFile((dataPath + "xtreme-flash.bin").c_str());

        // ram persistence for FS
        if(persistExtRAM)
            extRAM->loadFile((deviceDataPath + "exram.bin").c_str());

        // can still boot over serial
        if(!serialBootFile.empty())
            static_cast<BootSerial *>(bootSerial.get())->setBootFile(serialBootFile);

        if(usbip)
            usb->startEnumeration();
    }
    else
    {
        // this is a bit less stable as I don't actually have one

        flash = std::make_unique<MemoryDevice>(0x3FFFF);
        keyboard = std::make_unique<ClassicKeyboardDevice>();

        rtc = std::make_unique<PCF8593>(0, 1);

        cpu.setExternalArea(0, flash.get());
        cpu.setExternalArea(1, extRAM.get()); //external ram
        cpu.setExternalArea(3, lcd.get()); //lcd
        cpu.setExternalArea(7, keyboard.get()); // keyboard

        // serial ports
        serialFlash = std::make_unique<SerialFlash>();
        bootSerial = std::make_unique<BootSerial>(2);

        cpu.setSerialDevice(0, rfSerial.get());
        cpu.setSerialDevice(1, serialFlash.get());
        cpu.setSerialDevice(2, bootSerial.get());

        // IO ports
        port1 = std::make_unique<Port1Classic>();
        classicPort3 = std::make_unique<Port3Classic>(*serialFlash);
        classicPortF = std::make_unique<PortFClassic>();

        cpu.addIODevice(IOPort::_1, port1.get());
        cpu.addIODevice(IOPort::_3, classicPort3.get());
        cpu.addIODevice(IOPort::F, rtc.get());
        cpu.addIODevice(IOPort::F, classicPortF.get());

        // load rom/flash dumps
        // (These are from the "C4PC system pack")
        if(!cpu.loadROM((dataPath + "emu_rom.bin").c_str()))
        {
            std::cerr << "Failed to load ROM!\n";
            return 1;
        }

        // try to load updated file first, fallback to base flash
        if(!serialFlash->loadFile((dataPath + "classic-flash-persist.bin").c_str()))
            serialFlash->loadFile((dataPath + "emu_flash.bin").c_str());
        flash->loadFile((dataPath + "emu_cyos.bin").c_str());
    }

    // change id for fun
    if(xtreme)
    {
        uint32_t cyID = cyIDFromString(cyIDStr);

        flash->write(0x7F818, cyID >> 24);
        flash->write(0x7F819, cyID >> 16);
        flash->write(0x7F81A, cyID >> 8);
        flash->write(0x7F81B, cyID);
        uint32_t crc = ~crc32(flash->getData() + 0x7F800, 0x7FC);
        flash->write(0x7FFFC, crc >> 24);
        flash->write(0x7FFFD, crc >> 16);
        flash->write(0x7FFFE, crc >> 8);
        flash->write(0x7FFFF, crc);
    }


    cpu.reset();

    // set battery (needs to be after reset)
    if(xtreme)
    {
        setXtremeBatteryLevel(cpu, extRAM);

        if(mp3SD)
        {
            cpu.setADCValue(3, 0);
            bootSerial->getSD().setFile(deviceDataPath + "sdcard.bin");
        }
    }

    //rendering setup

    SDL_Window *sdlWindow;
    SDL_Renderer *sdlRenderer;
    sdlWindow = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1600, 1000, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_PRESENTVSYNC);
    SDL_RenderSetLogicalSize(sdlRenderer, 160, 100);

    auto texture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 160, 100);
    uint8_t screenBuffer[160 * 100 * 4]{0};

    // audio
    if(xtreme)
    {
        SDL_AudioSpec spec{};

        spec.freq = 48000;
        spec.format = AUDIO_S8;
        spec.channels = 1;
        spec.samples = 512;

        auto audioDevice = SDL_OpenAudioDevice(nullptr, false, &spec, nullptr, 0);

        if(!audioDevice)
            std::cerr << "Failed to open audio: " << SDL_GetError() << "\n";
        else
        {
            SDL_PauseAudioDevice(audioDevice, 0);

            static_cast<Port1Sound *>(port1.get())->setAudioDevice(audioDevice);
        }
    }

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

            // force audio update if TPU not running
            if(!cpu.getTPUStarted(1) && xtreme)
                static_cast<Port1Sound *>(port1.get())->updateAudio(cpu.getClock());

            if(benchmarkMode)
            {
                benchmarkCycles -= cpuCycles;
                if(benchmarkCycles < 0)
                    running = false;
            }
        }

        if(usbip)
            usb->usbipUpdate();

        rfSerial->networkUpdate();

        //update screen
        // TODO: sync
        lcd->convertDisplay(screenBuffer);
        SDL_UpdateTexture(texture, nullptr, screenBuffer, 160 * 4);
        SDL_RenderCopy(sdlRenderer, texture, nullptr, nullptr);
        SDL_RenderPresent(sdlRenderer);
    }

    if(persistExtRAM && !benchmarkMode)
    {
        if(xtreme)
            extRAM->saveFile((deviceDataPath + "exram.bin").c_str());
        else
            serialFlash->saveFile((dataPath + "classic-flash-persist.bin").c_str());
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(sdlRenderer);
    SDL_DestroyWindow(sdlWindow);

    return 0;
}
