#ifndef H8CPU_H
#define H8CPU_H

#include <cstdint>
#include <list>

enum class IOPort
{
    _1 = 0,
    _2,
    _3,
    _4,
    _5,
    _6,
    A = 9,
    B,
    C,
    D,
    E,
    F,
    G
};

enum class InterruptSource
{
    Invalid = -1,

    Reset = 0,
    // 1-4 reserved
    Trace = 5,
    // 6 reserved
    NMI = 7,
    Trap0,
    Trap1,
    Trap2,
    Trap3,
    // 12-15 reserved
    IRQ0 = 16,
    IRQ1,
    IRQ2,
    IRQ3,
    IRQ4,
    IRQ5,
    IRQ6,
    IRQ7,
    SWDTEND,
    WOVI,
    CMI,
    // 27 reserved
    ADI = 28,
    // 29-31 reserved
    TGI0A = 32,
    TGI0B,
    TGI0C,
    TGI0D,
    TGI0V,
    // 37-39 reserved
    TGI1A = 40,
    TGI1B,
    TGI1V,
    TGI1U,
    TGI2A,
    TGI2B,
    TGI2V,
    TGI2U,
    TGI3A,
    TGI3B,
    TGI3C,
    TGI3D,
    TGI3V,
    // 53-55 reserved
    TGI4A = 56,
    TGI4B,
    TGI4V,
    TGI4U,
    TGI5A,
    TGI5B,
    TGI5V,
    TGI5U,
    CMIA0,
    CMIB0,
    OVI0,
    // 67 reserved
    CMIA1 = 68,
    CMIB1,
    OVI1,
    // 71 reserved
    DEND0A = 72,
    DEND0B,
    DEND1A,
    DEND1B,
    // 76-79 reserved
    ERI0 = 80,
    RXI0,
    TXI0,
    TEI0,
    ERI1,
    RXI1,
    TXI1,
    TEI1,
    ERI2,
    RXI2,
    TXI2,
    TEI2,
};

class ExternalAddressDevice
{
public:
    virtual ~ExternalAddressDevice() = default;

    virtual uint8_t read(uint32_t addr) = 0;
    virtual void write(uint32_t addr, uint8_t val) = 0;

    virtual uint8_t *getPtr(uint32_t &mask){return nullptr;}
};

class IODevice
{
public:
    virtual ~IODevice() = default;

    virtual uint8_t read() = 0;
    virtual void write(uint8_t val) = 0;
    virtual void setDirection(uint8_t dir) = 0;
};

class SerialDevice
{
public:
    virtual ~SerialDevice() = default;

    virtual uint8_t read() = 0;
    virtual void write(uint8_t val) = 0;
    virtual bool canRead() = 0;
};

class H8CPU final
{
public:
    H8CPU();

    bool loadROM(const char *filename);
    void reset();

    void interrupt(InterruptSource source);
    void externalInterrupt(int num); // IRQn

    bool executeCycles(int cycles);
    int executeInstruction();

    void dumpRegs() const;

    void setExternalArea(int area, ExternalAddressDevice *device);
    void addIODevice(IOPort port, IODevice *device);
    void setSerialDevice(int index, SerialDevice *device);

    void setADCValue(unsigned channel, uint16_t value);

    bool getSleeping() const;
    uint64_t getClock() const;

    uint8_t *getRAM() {return ram;}

protected:
    using B = uint8_t;
    using W = uint16_t;
    using L = uint32_t;

    class DMAChannel
    {
    public:
        DMAChannel(int channel) : channel(channel){}

        uint8_t getReg(int addr) const;
        void setReg(int addr, uint8_t val);

        void transfer(H8CPU &cpu);

        const int channel;

        uint32_t memAddrA = 0, memAddrB = 0;
        uint16_t ioAddrA = 0, ioAddrB = 0;
        uint16_t countA = 0, countB = 0;
        uint16_t control = 0;

        uint16_t bandControl = 0; // same register for both
    };

    class IOPort
    {
    public:
        IOPort(int index) : index(index) {}

        uint8_t read() const;
        void write(uint8_t val);
        uint8_t getWrittenData() const;

        void setDirection(uint8_t dir);
        uint8_t getDirection() const;

        void addDevice(IODevice *device);

    protected:
        const int index;

        bool log = false;
        uint8_t data = 0;
        //uint8_t readData = 0;
        uint8_t direction = 0;

        std::list<IODevice *> devices;
    };

    class TPU
    {
    public:
        TPU(int index) : index(index) {}

        uint8_t getReg(int addr) const;
        void setReg(int addr, uint8_t val);

        void update(H8CPU &cpu, int cycles);

    protected:
        void calcNextUpdate();

        const int index = 0;

        int clockDiv = 0, clockShift = 0;
        int frac = 0;

        int nextUpdate = 0;
        int clearOn = 0;

        uint8_t interruptEnable = 0;
        uint8_t status = 0;
        uint16_t counter = 0;
        uint16_t general[4]{0xFFFF}; // 0/3 have 4, others have 2
    };

    class Timer
    {
    public:
        Timer(int index) : index(index) {}

        uint8_t getReg(int addr) const;
        void setReg(int addr, uint8_t val);

        void update(H8CPU &cpu, int clocks);

    protected:
        const int index;

        uint8_t control = 0;
        uint8_t controlStatus = 0;
        uint8_t constantA = 0, constantB = 0;
        uint8_t counter = 0;

        int clockDiv = 0;
        int frac = 0;
    };

    class Serial
    {
    public:
        Serial(int index) : index(index) {}

        uint8_t getReg(int addr) const;
        void setReg(int addr, uint8_t val);

        void update(H8CPU &cpu);

        void setDevice(SerialDevice *device);

    protected:
        const int index;
        uint8_t mode = 0, bitRate = 0, control = 0, status = 0x84, smartCardMode = 0;

        uint8_t txData = 0, rxData = 0;

        SerialDevice *device = nullptr;

        bool forceTXI = false;
    };

    void serviceInterrupt();

    bool updateDTC();
    void updateADC(int cycles);

    // some of the longer encodings
    inline int handleLongMOV0100();
    inline int handleWordLDCSTC();
    inline int handleLong01F0();
    inline int handleADDSINC(uint8_t b1);
    inline int handleLeftShift(uint8_t b1);
    inline int handleRightShift(uint8_t b1);
    inline int handleLeftRotate(uint8_t b1);
    inline int handleRightRotate(uint8_t b1);
    inline int handleNOTEXTNEG(uint8_t b1);
    inline int handleSUBSDEC(uint8_t b1);
    inline int handleBranch(uint8_t condition, int offset);
    inline int handle6A(uint8_t b1);
    inline int handle6B(uint8_t b1);
    inline int handleMOV78(uint8_t b1);
    inline int handleWord79(uint8_t b1);
    inline int handleLong7A(uint8_t b1);

    // bit ops that only set flags
    inline bool handleBitFlags(uint8_t op, uint32_t addr, int bitVal);
    // bit ops that write a value (and don't set flags)
    inline bool handleBitValue(uint8_t op, uint32_t addr, int bitVal);

    template<class T>
    inline void doADD(T a, T &b);
    template<class T>
    inline void doADDX(T a, T &b);
    template<class T>
    inline void doAND(T a, T &b);
    template<class T>
    inline void doCMP(T a, T b);
    template<class T, int n = 1>
    inline void doDEC(T &r);
    template<class T, int n = 1>
    inline void doINC(T &r);
    template<class T>
    inline T doMOV(T val);
    template<class T>
    inline void doOR(T a, T &b);
    template<class T, int s = 1>
    inline void doROTL(T &r);
    template<class T, int s = 1>
    inline void doROTR(T &r);
    template<class T, int s = 1>
    inline void doROTXL(T &r);
    template<class T, int s = 1>
    inline void doROTXR(T &r);
    template<class T, int s = 1>
    inline void doSHAL(T &r);
    template<class T, int s = 1>
    inline void doSHAR(T &r);
    template<class T, int s = 1>
    inline void doSHLL(T &r);
    template<class T, int s = 1>
    inline void doSHLR(T &r);
    template<class T>
    inline void doSUB(T a, T &b);
    template<class T>
    inline void doSUBX(T a, T &b);
    template<class T>
    inline void doXOR(T a, T &b);

    inline void setPC(uint32_t addr, bool isCall = false, bool isRet = false);

    template<class T>
    inline void updateFlags(T val);

    inline void updateFlag(int flag, bool set);

    // timing helpers
    int fetchTiming(int len) const;
    int byteAccessTiming(uint32_t addr) const;
    int wordAccessTiming(uint32_t addr) const;

    void updateBusTimings();

    // reg helpers
    template<class T>
    inline T reg(int r) const;
    template<class T>
    inline T &reg(int r);

    uint8_t readByte(uint32_t addr) const;
    uint16_t readWord(uint32_t addr) const;
    uint32_t readLong(uint32_t addr) const;

    // 16-bit address/displacement
    inline uint32_t readAddr16(uint32_t addr) const;
    inline uint32_t readDisp16(uint32_t addr) const;

    // memory
    void writeByte(uint32_t addr, uint8_t val);
    void writeWord(uint32_t addr, uint16_t val);
    void writeLong(uint32_t addr, uint32_t val);

    uint8_t readIOReg(uint32_t addr) const;
    void writeIOReg(uint32_t addr, uint8_t val);

    uint32_t er[8]{0};
    uint32_t pc;
    uint8_t ccr;

    uint8_t *areaPtrs[8]{nullptr};
    uint32_t areaMasks[8]{0x1FFFFF};
    int areaAccessCycles8[8];
    int areaAccessCycles16[8];

    static const int romSize = 0x10000; //assume EAE=1
    uint8_t rom[romSize]{0};
    uint8_t ram[0x2000]{0};

    ExternalAddressDevice *externalAreas[8]{nullptr};

    // bus control
    uint8_t busWidthControl = 0xFF;
    uint8_t busWaitControl = 0xFF;
    uint16_t busWaits = 0xFFFF;

    // interrupt regs
    uint8_t irqEnable, irqStatus;

    // interrupt requests
    uint64_t requestedInterrupts[2]{0};

    uint16_t mstpcr;

    DMAChannel dmaChannels[2];

    // dtc
    uint8_t dtcEnable[6];

    IOPort ioPorts[16]; // 6-8 don't exist
    TPU tpuChannels[6];
    uint8_t tpuStart; //TSTR
    Timer timers[2];
    Serial serialPorts[3];

    // adc
    uint8_t a2dControlStatus = 0;
    int a2dConversionTime = 266, a2dConvCounter = 266;
    int a2dChannel = 0, a2dEndChannel = 0;
    uint16_t a2dData[4]{0};

    uint16_t a2dValues[8]; // raw 10-bit values

    bool sleeping;
    uint64_t clock;
};

#endif
