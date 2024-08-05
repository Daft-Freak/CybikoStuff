// split out 15 dec 2019
// probably written in jan
#include <cassert>
#include <fstream>
#include <iostream>

#include "H8CPU.h"
#include "Registers.h"

enum ConditionCode
{
    CC_C = (1 << 0),
    CC_V = (1 << 1),
    CC_Z = (1 << 2),
    CC_N = (1 << 3),
    CC_U = (1 << 4),
    CC_H = (1 << 5),
    CC_UI = (1 << 6),
    CC_I = (1 << 7)
};

// reg specialisations
template<>
uint8_t H8CPU::reg(int r) const;
template<>
uint8_t &H8CPU::reg(int r);
template<>
uint16_t H8CPU::reg(int r) const;
template<>
uint16_t &H8CPU::reg(int r);
template<>
uint32_t H8CPU::reg(int r) const;
template<>
uint32_t &H8CPU::reg(int r);

H8CPU::H8CPU() : pc(0), ccr(0), mstpcr(0x3FFF), dmaChannels{0, 1}, ioPorts{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}, tpuChannels{0, 1, 2, 3, 4, 5}, tpuStart(0)
               , timers{0, 1}, serialPorts{0, 1, 2}, sleeping(false), clock(0)
{

}

bool H8CPU::loadROM(const char *filename)
{
    std::ifstream file(filename);
    if(!file)
        return false;

    file.read(reinterpret_cast<char *>(rom), romSize);

    std::cout << "Read " << file.gcount() << " bytes into ROM" << std::endl;

    return true;
}

void H8CPU::reset()
{
    pc = (rom[0] << 24) | (rom[1] << 16) | (rom[2] << 8) | rom [3]; //read the reset vector
    ccr |= 0x80;
    sleeping = false;

    irqEnable = irqStatus = 0;

    for(auto &en : dtcEnable)
        en = 0;

    busWaitControl = 0xFF;
    busWidthControl = 0xFF;
    busWaits = 0xFFFF;

    updateBusTimings();

    //TODO: reset peripherals...

    for(auto &v : a2dValues)
        v = 0x1FF; // ~half
}

void H8CPU::interrupt(InterruptSource source)
{
    int srcIdx = static_cast<int>(source);

    if(srcIdx < 64)
        requestedInterrupts[0] |= 1ull << srcIdx;
    else
        requestedInterrupts[1] |= 1ull << (srcIdx - 64);
}

void H8CPU::externalInterrupt(int num)
{
    if(num < 0 || num > 7)
        return;

    irqStatus |= 1 << num;

    if(irqEnable & (1 << num))
        interrupt(static_cast<InterruptSource>(static_cast<int>(InterruptSource::IRQ0) + num));
}

bool H8CPU::executeCycles(int cycles)
{
    while(cycles > 0)
    {
        int executed = 0;

        // DMAC/DTC can be activated by interrupts, so do these right before serviceInterrupt
        if(!(mstpcr & MSTPCR_DMAC))
        {
            // this is actually the same register...
            if(dmaChannels[0].bandControl & 0x30)
                executed += dmaChannels[0].transfer(*this);

            if(dmaChannels[0].bandControl & 0xC0)
                executed += dmaChannels[1].transfer(*this);
        }

        bool dtcTriggered = false;

        if(!(mstpcr & MSTPCR_DTC))
            dtcTriggered = updateDTC();

        if(!dtcTriggered)
            serviceInterrupt();

        // execute CPU instruction if not sleeping
        int instrExec = 1; // TODO: optimise sleeping
        if(!sleeping && !(instrExec = executeInstruction()))
            return false;

        executed += instrExec;

        cycles -= executed;
        clock += executed;

        // peripheral update
        // TODO: don't update everything every instruction

        if(!(mstpcr & MSTPCR_TPU))
        {
            auto tpuMask = tpuInterruptEnable & tpuStart;
            for(int i = 0; tpuMask; i++, tpuMask >>= 1)
            {
                if(tpuMask & 1)
                    tpuChannels[i].updateForInterrupts(*this);
            }
        }

        if(!(mstpcr & MSTPCR_Timer8b))
        {
            timers[0].update(*this, executed);
            timers[1].update(*this, executed);
        }

        //if(clock & 8) // slow it down a bit
        {
            if(!(mstpcr & MSTPCR_SCI0))
                serialPorts[0].update(*this);
            if(!(mstpcr & MSTPCR_SCI1))
                serialPorts[1].update(*this);
            if(!(mstpcr & MSTPCR_SCI2))
                serialPorts[2].update(*this);
        }

        if(!(mstpcr & MSTPCR_A2D))
            updateADC(executed);
    }
    return true;
}

int H8CPU::executeInstruction()
{
    uint8_t b0 = readByte(pc);
    uint8_t b1 = readByte(pc + 1);
    pc += 2; //no instruction is shorter than this

    // 8bit immediate instructions
    switch(b0 >> 4)
    {
        case 0x2: //mov.b addr:8 reg
            reg<B>(b0 & 0xF) = doMOV(readByte(b1 | 0xFFFF00));
            return fetchTiming(1) + byteAccessTiming(b1 | 0xFFFF00);

        case 0x3: //mov.b reg addr:8
            writeByte(b1 | 0xFFFF00, doMOV(reg<B>(b0 & 0xF)));
            return fetchTiming(1) + byteAccessTiming(b1 | 0xFFFF00);

        case 0x4:
            return handleBranch(b0 & 0xF, static_cast<int8_t>(b1));

        case 0x8: //add.b imm reg
            doADD(b1, reg<B>(b0 & 0xF));
            return fetchTiming(1);

        case 0x9: //addx imm reg
            doADDX(b1, reg<B>(b0 & 0xF));
            return fetchTiming(1);

        case 0xA: //cmp.b imm reg
            doCMP(b1, reg<B>(b0 & 0xF));
            return fetchTiming(1);

        case 0xB: //subx imm reg
            doSUBX(b1, reg<B>(b0 & 0xF));
            return fetchTiming(1);

        case 0xC: //or.b imm reg
            doOR(b1, reg<B>(b0 & 0xF));
            return 1;

        case 0xD: //xor.b imm reg
            doXOR(b1, reg<B>(b0 & 0xF));
            return 1;

        case 0xE: //and.b imm reg
            doAND(b1, reg<B>(b0 & 0xF));
            return fetchTiming(1);

        case 0xF: //mov.b imm reg
            reg<B>(b0 & 0xF) = doMOV(b1);
            return fetchTiming(1);
    }

    // second byte is frequently two registers
    auto b1h = b1 >> 4;  // src
    auto b1l = b1 & 0xF; // dst

    switch(b0)
    {
        case 0x00: //nop
            assert(b1 == 0);
            return fetchTiming(1);

        case 0x01:
        {
            switch(b1h)
            {
                case 0x0: // mov
                    assert(b1l == 0);
                    return handleLongMOV0100();
                case 0x1:
                case 0x2: // ldm/stm
                case 0x3:
                {
                    auto b3 = readByte(pc + 1);

                    assert(b1l == 0);
                    assert(readByte(pc) == 0x6D);
                    assert((b3 & 0x70) == 0x70);

                    int startReg = b3 & 0xF;
                    int numRegs = (b1 >> 4) + 1; //01 10 -> 2, 01 20 -> 3, 01 30 -> 4
                    pc += 2;

                    if(b3 & 0x80) //stm.l reg-reg @-sp
                    {
                        for(int i = 0; i < numRegs; i++)
                        {
                            er[7] -= 4;
                            writeLong(er[7], er[startReg + i]);
                        }
                    }
                    else //ldm.l @sp+ reg-reg
                    {
                        for(int i = 0; i < numRegs; i++)
                        {
                            er[startReg - i] = readLong(er[7]);
                            er[7] += 4;
                        }
                    }

                    return fetchTiming(2) + wordAccessTiming(er[7]) * numRegs * 2 + 1;
                }
                case 0x4: // ldc/stc.w
                    return handleWordLDCSTC();
                // 5 invalid
                //case 0x6: // mac (invalid)
                // 7 invalid
                case 0x8: // sleep
                    assert(b1l == 0);
                    sleeping = true;
                    return fetchTiming(1) + 1;
                // 9 invalid
                //case 0xA: // clrmac (invalid)
                // B invalid
                case 0xC: // mulxs
                {
                    assert(b1l == 0);
                    auto b2 = readByte(pc);
                    auto b3 = readByte(pc + 1);
                    pc += 2;

                    if(b2 == 0x50) // mulxs.b
                    {
                        auto a = static_cast<int8_t>(reg<W>(b3 & 0xF) & 0xFF);
                        auto b = static_cast<int8_t>(reg<B>(b3 >> 4));
                        auto res = static_cast<uint16_t>(static_cast<int16_t>(a) * b);

                        reg<W>(b3 & 0xF) = res;
                        updateFlags(res);
                        return fetchTiming(2) + 11;
                    }
                    else if(b2 == 0x52) // mulxs.w
                    {
                        auto dest = b3 & 0x7;
                        auto a = static_cast<int16_t>(er[dest] & 0xFFFF);
                        auto b = static_cast<int16_t>(reg<W>(b3 >> 4));

                        er[dest] = static_cast<uint32_t>(static_cast<int32_t>(a) * b);
                        updateFlags(er[dest]);
                        return fetchTiming(2) + 19;
                    }

                    assert(!"Invalid opcode!");
                    return 0;
                }
                case 0xD: // divxs
                {
                    assert(b1l == 0);
                    auto b2 = readByte(pc);
                    auto b3 = readByte(pc + 1);
                    pc += 2;

                    if(b2 == 0x51) // divxs.b
                    {
                        auto dividend = static_cast<int16_t>(reg<W>(b3 & 0xF));
                        auto divisor = static_cast<int8_t>(reg<B>(b3 >> 4));

                        int16_t quot = dividend >> 8, rem = dividend;

                        if(divisor == -1 && dividend == -32768) // undefined behaviour
                            quot = rem = -1;
                        else if(divisor != 0)
                        {
                            quot = dividend / divisor;
                            rem %= divisor;
                        }
                        else if(quot < 0) // divide negative by 0
                            quot = quot + 1; // negative vaules end up as + 1

                        updateFlag(CC_Z, divisor == 0);
                        // N flag is set if the result should be negative, not if it actually is
                        updateFlag(CC_N, (dividend < 0) != (divisor < 0));

                        reg<W>(b3 & 0xF) = ((static_cast<uint16_t>(rem) & 0xFF) << 8) | (static_cast<uint16_t>(quot) & 0xFF);

                        return fetchTiming(2) + 11;
                    }
                    else if(b2 == 0x53) // divxs.w
                    {
                        auto dest = b3 & 0x7;
                        auto dividend = static_cast<int32_t>(er[dest]);
                        auto divisor = static_cast<int16_t>(reg<W>(b3 >> 4));

                        int32_t quot = dividend >> 16, rem = dividend;

                        if(divisor == -1 && dividend == -2147483648) // undefined behaviour
                            quot = rem = -1;
                        else if(divisor != 0)
                        {
                            quot = dividend / divisor;
                            rem %= divisor;
                        }
                        else if(quot < 0) // divide negative by 0
                            quot = quot + 1; // negative vaules end up as + 1

                        updateFlag(CC_Z, divisor == 0);
                        // N flag is set if the result should be negative, not if it actually is
                        updateFlag(CC_N, (dividend < 0) != (divisor < 0));

                        er[dest] = ((static_cast<uint32_t>(rem) & 0xFFFF) << 16) | (static_cast<uint32_t>(quot) & 0xFFFF);

                        return fetchTiming(2) + 19;
                    }

                    assert(!"Invalid opcode!");
                    return 0;
                }
                case 0xE: // tas
                {
                    assert(b1l == 0);

                    auto b2 = readByte(pc);
                    auto b3 = readByte(pc + 1);
                    pc += 2;

                    if(b2 == 0x7B)
                    {
                        assert((b3 & 0xF) == 0xC);
                        auto reg = (b3 >> 4) & 0x7; // only 0, 1, 4, 5 valid
                        uint8_t val = readByte(er[reg]);

                        updateFlags(val);
                        updateFlag(CC_V, false);

                        writeByte(er[reg], val | 0x80);

                        return fetchTiming(2) + byteAccessTiming(er[reg]) * 2;
                    }

                    assert(!"Invalid opcode!");
                    return 0;
                }
                case 0xF: // or/xor/and .l r r
                    assert(b1l == 0);
                    return handleLong01F0();

                default:
                    return 0;
            }
        }

        case 0x02: //stc
            reg<B>(b1) = ccr;
            return fetchTiming(1);

        case 0x03: //ldc
            ccr = reg<B>(b1);
            return fetchTiming(1);

        case 0x04: //orc
            ccr = ccr | b1;
            return fetchTiming(1);

        case 0x05: //xorc
            ccr = ccr ^ b1;
            return fetchTiming(1);

        case 0x06: //andc
            ccr = ccr & b1;
            return fetchTiming(1);

        case 0x07: // ldc imm
            ccr = b1;
            return fetchTiming(1);

        case 0x08: //add.b r r
            doADD(reg<B>(b1h), reg<B>(b1l));
            return fetchTiming(1);

        case 0x09: //add.w r r
            doADD(reg<W>(b1h), reg<W>(b1l));
            return fetchTiming(1);

        case 0x0A:
        {
            if(b1 & 0x80) //add.l reg reg
                doADD(reg<L>(b1h & 7), reg<L>(b1l));
            else // inc.b
                doINC(reg<B>(b1l));

            return fetchTiming(1);
        }

        case 0x0B:
            return handleADDSINC(b1);

        case 0x0C: //mov.b reg reg
            reg<B>(b1l) = doMOV(reg<B>(b1h));
            return fetchTiming(1);

        case 0x0D: //mov.w reg reg
            reg<W>(b1l) = doMOV(reg<W>(b1h));
            return fetchTiming(1);

        case 0x0E: //addx reg reg
            doADDX(reg<B>(b1h), reg<B>(b1l));
            return fetchTiming(1);

        case 0x0F:
        {
            if(b1 & 0x80) //mov.l reg reg
            {
                er[b1l] = doMOV(er[b1h & 0x7]);
                return fetchTiming(1);
            }
            else // daa
            {
                uint8_t val = reg<B>(b1l);

                if((ccr & CC_C) || val > 0x99)
                {
                    val += 0x60;
                    ccr |= CC_C;
                }

                if((ccr & CC_H) || (val & 0x0F) > 0x09)
                    val += 0x06;

                reg<B>(b1l) = val;
                updateFlags(val);

                // H/V are now unspecified

                return fetchTiming(1);
            }
        }

        case 0x10:
            return handleLeftShift(b1);

        case 0x11:
            return handleRightShift(b1);

        case 0x12:
            return handleLeftRotate(b1);

        case 0x13:
            return handleRightRotate(b1);

        case 0x14: //or.b reg reg
            doOR(reg<B>(b1h), reg<B>(b1l));
            return fetchTiming(1);

        case 0x15: //xor.b reg reg
            doXOR(reg<B>(b1h), reg<B>(b1l));
            return fetchTiming(1);

        case 0x16: //and.b reg reg
            doAND(reg<B>(b1h), reg<B>(b1l));
            return fetchTiming(1);

        case 0x17:
            return handleNOTEXTNEG(b1);

        case 0x18: //sub.b r r
            doSUB(reg<B>(b1h), reg<B>(b1l));
            return fetchTiming(1);

        case 0x19: //sub.w r r
            doSUB(reg<W>(b1h), reg<W>(b1l));
            return fetchTiming(1);

        case 0x1A:
        {
            if(b1 & 0x80) //sub.l r r
                doSUB(er[b1h & 0x7], er[b1l]);
            else // dec.b
                doDEC(reg<B>(b1l));
            return fetchTiming(1);
        }

        case 0x1B:
            return handleSUBSDEC(b1);

        case 0x1C: //cmp.b r r
            doCMP(reg<B>(b1h), reg<B>(b1l));
            return fetchTiming(1);

        case 0x1D: //cmp.w r r
            doCMP(reg<W>(b1h), reg<W>(b1l));
            return fetchTiming(1);

        case 0x1E: //subx reg reg
            doSUBX(reg<B>(b1h), reg<B>(b1l));
            return fetchTiming(1);

        case 0x1F:
        {
            if(b1 & 0x80) // cmp.l r r
            {
                doCMP(er[b1h & 0x7], er[b1l]);
                return fetchTiming(1);
            }
            else // das
            {
                uint8_t val = reg<B>(b1l);

                if(ccr & CC_C)
                    val -= 0x60;
                if(ccr & CC_H)
                    val -= 0x06;

                reg<B>(b1l) = val;
                updateFlags(val);

                // H/V are now unspecified

                return fetchTiming(1);
            }
        }

        // 2x, 3x, 4x handled above

        case 0x50: //mulxu.b
            reg<W>(b1l) = (reg<W>(b1l) & 0xFF) * reg<B>(b1h);
            return fetchTiming(1) + 11;

        case 0x51: //divxu.b
        {
            auto divisor = reg<B>(b1h);
            auto dividend = reg<W>(b1l);
            updateFlags(divisor);

            uint16_t quot = dividend >> 8, rem = dividend;

            if(divisor != 0)
            {
                quot = dividend / divisor;
                rem %= divisor;
            }

            reg<W>(b1l) = ((rem & 0xFF) << 8) | (quot & 0xFF);
            return fetchTiming(1) + 11;
        }

        case 0x52: //mulxu.w
            er[b1l] = (er[b1l] & 0xFFFF) * reg<W>(b1h);
            return fetchTiming(1) + 19;

        case 0x53: //divxu.w
        {
            auto divisor = reg<W>(b1h);
            updateFlags(divisor);

            // divide by zero seems to result in swapping the two words
            uint32_t quot = er[b1l] >> 16, rem = er[b1l];

            if(divisor != 0)
            {
                quot = er[b1l] / divisor;
                rem %= divisor;
            }

            er[b1l] = ((rem & 0xFFFF) << 16) | (quot & 0xFFFF);
            return fetchTiming(1) + 19;
        }

        case 0x54: //rts
            assert(b1 == 0x70);
            setPC(readLong(er[7]) & 0xFFFFFF, false, true);
            er[7] += 4;
            return fetchTiming(2) + wordAccessTiming(er[7]) * 2 + 1; // advanced mode

        case 0x55: //bsr 8
        {
            //push pc to stack
            er[7] -= 4;
            writeLong(er[7], pc);

            setPC(pc + static_cast<int8_t>(b1), true);
            return fetchTiming(2) + wordAccessTiming(er[7]) * 2; // adv
        }

        case 0x56: //rte
        {
            assert(b1 == 0x70);
            auto val = readLong(er[7]);
            pc = val & 0xFFFFFF;
            ccr = val >> 24;
            er[7] += 4;

            return fetchTiming(2) + wordAccessTiming(er[7]) * 2 + 1; // no exr
        }

        // 57: trapa

        case 0x58: // bxx :16
            pc += 2;
            return handleBranch(b1h, static_cast<int16_t>(readWord(pc - 2))) + 1 /*16 bit branches have an internal op*/;

        case 0x59: //jmp @reg
            setPC(er[b1h]);
            return fetchTiming(2);

        case 0x5A: //jmp :24
            setPC(readWord(pc) | (b1 << 16));
            return fetchTiming(2) + 1;

        // 5B: jmp @@:8 (mem indirect)

        case 0x5C: //bsr :16
        {
            pc += 2;

            //push pc to stack
            er[7] -= 4;
            writeLong(er[7], pc);

            auto disp = static_cast<int16_t>(readWord(pc - 2));
            setPC(pc + disp, true);
            return fetchTiming(2) + wordAccessTiming(er[7]) * 2 + 1; // adv
        }

        case 0x5D: //jsr @reg
            //push pc to stack
            er[7] -= 4;
            writeLong(er[7], pc);

            setPC(er[b1h], true);
            return fetchTiming(2) + wordAccessTiming(er[7]) * 2; // adv

        case 0x5E: //jsr :24
        {
            uint32_t addr = readWord(pc) | (b1 << 16);
            pc += 2;

            //push pc to stack
            er[7] -= 4;
            writeLong(er[7], pc);

            //jump
            setPC(addr, true);
            return fetchTiming(2) + wordAccessTiming(er[7]) * 2 + 1; // adv mode
        }

        // 5F: jsr @@:8 (mem indirect)
        case 0x60: //bset reg reg
        {
            auto bit = reg<B>(b1h) & 0x7;
            reg<B>(b1l) = reg<B>(b1l) | (1 << bit);
            return fetchTiming(1);
        }

        case 0x61: //bnot reg reg
        {
            auto bit = reg<B>(b1h) & 0x7;
            reg<B>(b1l) = reg<B>(b1l) ^ (1 << bit);
            return fetchTiming(1);
        }

        case 0x62: //bclr reg reg
        {
            auto bit = reg<B>(b1h) & 0x7;
            reg<B>(b1l) = reg<B>(b1l) & ~(1 << bit);
            return fetchTiming(1);
        }

        case 0x63: //btst reg reg
        {
            auto bit = reg<B>(b1h) & 0x7;
            updateFlag(CC_Z, (reg<B>(b1l) & (1 << bit)) == 0);
            return fetchTiming(1);
        }

        case 0x64: //or.w reg reg
            doOR(reg<W>(b1h), reg<W>(b1l));
            return fetchTiming(1);

        case 0x65: //xor.w reg reg
            doXOR(reg<W>(b1h), reg<W>(b1l));
            return fetchTiming(1);

        case 0x66: //and.w reg reg
            doAND(reg<W>(b1h), reg<W>(b1l));
            return fetchTiming(1);

        case 0x67: //bst/bist reg
        {
            auto bit = b1h & 0x7;
            bool v = ccr & CC_C;

            if(b1h & 0x8) // bist
                v = !v;

            if(v)
                reg<B>(b1l) = reg<B>(b1l) | (1 << bit);
            else
                reg<B>(b1l) = reg<B>(b1l) & ~(1 << bit);

            return fetchTiming(1);
        }

        case 0x68:
            if(b1 & 0x80) //mov.b reg @reg
                writeByte(er[b1h & 0x7], doMOV(reg<B>(b1l)));
            else //mov.b @reg reg
                reg<B>(b1l) = doMOV(readByte(er[b1h & 0x7]));

            return fetchTiming(1) + byteAccessTiming(er[b1h & 0x7]);

        case 0x69:
            if(b1 & 0x80) //mov.w reg @reg
                writeWord(er[b1h & 0x7], doMOV(reg<W>(b1l)));
            else //mov.w @reg reg
                reg<W>(b1l) = doMOV(readWord(er[b1h & 0x7]));

            return fetchTiming(1) + wordAccessTiming(er[b1h & 0x7]);

        case 0x6A:
            return handle6A(b1);

        case 0x6B:
            return handle6B(b1);

        case 0x6C:
        {
            auto regA = b1h & 0x7;

            if(b1 & 0x80) //mov.b reg @-reg
            {
                er[regA]--;
                writeByte(er[regA], doMOV(reg<B>(b1l)));
            }
            else //mov.b @reg+ reg
            {
                auto val = doMOV(readByte(er[regA]));
                er[regA]++;
                reg<B>(b1l) = val;
            }
            return fetchTiming(1) + byteAccessTiming(er[regA]) + 1;
        }

        case 0x6D:
        {
            auto regA = b1h & 0x7;

            if(b1 & 0x80) //mov.w reg @-reg
            {
                er[regA] -= 2;
                writeWord(er[regA], doMOV(reg<W>(b1l)));
            }
            else //mov.w @reg+ reg
            {
                auto val = doMOV(readWord(er[regA]));
                er[regA] += 2;
                reg<W>(b1l) = val;
            }
            return fetchTiming(1) + wordAccessTiming(er[regA]) + 1;
        }

        case 0x6E:
        {
            auto addr = er[b1h & 0x7] + readDisp16(pc);
            if(b1 & 0x80) //mov.b reg @reg + off:16
                writeByte(addr, doMOV(reg<B>(b1l)));
            else //mov.b @reg + off:16 reg
                reg<B>(b1l) = doMOV(readByte(addr));
            pc += 2;

            return fetchTiming(2) + byteAccessTiming(addr);
        }

        case 0x6F:
        {
            auto addr = er[b1h & 0x7] + readDisp16(pc);
            if(b1 & 0x80) //mov.w reg @reg + off:16
                writeWord(addr, doMOV(reg<W>(b1l)));
            else //mov.w @reg + off:16 reg
                reg<W>(b1l) = doMOV(readWord(addr));

            pc += 2;
            return fetchTiming(2) + wordAccessTiming(addr);
        }

        case 0x70: //bset
        {
            reg<B>(b1l) = reg<B>(b1l) | (1 << b1h);
            return fetchTiming(1);
        }

        case 0x71: //bnot
        {
            reg<B>(b1l) = reg<B>(b1l) ^ (1 << b1h);
            return fetchTiming(1);
        }

        case 0x72: //bclr
        {
            reg<B>(b1l) = reg<B>(b1l) & ~(1 << b1h);
            return fetchTiming(1);
        }

        case 0x73: //btst
        {
            updateFlag(CC_Z, (reg<B>(b1l) & (1 << b1h)) == 0);
            return fetchTiming(1);
        }

        case 0x74: //bor/bior imm reg
        {
            auto bit = b1h & 0x7;
            bool c = ccr & CC_C;
            bool v = reg<B>(b1l) & (1 << bit);

            if(b1h & 0x8) // bior
                v = !v;

            updateFlag(CC_C, v || c);

            return fetchTiming(1);
        }

        case 0x75: //bxor/bixor imm reg
        {
            auto bit = b1h & 0x7;
            bool c = ccr & CC_C;
            bool v = reg<B>(b1l) & (1 << bit);

            if(b1h & 0x8) // bixor
                v = !v;

            updateFlag(CC_C, v != c);

            return fetchTiming(1);
        }


        case 0x76: //band/biand imm reg
        {
            auto bit = b1h & 0x7;
            bool c = ccr & CC_C;
            bool v = reg<B>(b1l) & (1 << bit);

            if(b1h & 0x8) // biand
                v = !v;

            updateFlag(CC_C, v && c);

            return fetchTiming(1);
        }

        case 0x77: //bld/bild imm reg
        {
            auto bit = 1 << (b1h & 0x7);
            if(b1h & 0x8) // bild
                updateFlag(CC_C, !(reg<B>(b1l) & bit));
            else // bld
                updateFlag(CC_C, reg<B>(b1l) & bit);

            return fetchTiming(1);
        }

        case 0x78:
            return handleMOV78(b1);

        case 0x79:
            return handleWord79(b1);

        case 0x7A:
            return handleLong7A(b1);

        // 7B: eepmov
        case 0x7B:
        {
            assert(readByte(pc) == 0x59 && readByte(pc + 1) == 0x8F);
            assert(b1 == 0x5C || b1 == 0xD4);

            pc += 2;

            int len = er[4];

            if(b1 == 0xD4)
                len &= 0xFFFF; // .w
            else
                len &= 0xFF; // .b

            // hmm, interrupts should happen during this (only for .w)
            int states = fetchTiming(2) + byteAccessTiming(er[6]) * len + byteAccessTiming(er[5]) * (len + 1);

            while(len > 0)
            {
                writeByte(er[6]++, readByte(er[5]++));
                len--;
            }

            if(b1 == 0xD4)
                er[4] &= 0xFFFF0000; //set to 0
            else
                er[4] &= 0xFFFFFF00;

            return states;
        }

        case 0x7C:
        {
            auto b2 = readByte(pc);
            auto b3 = readByte(pc + 1);
            pc += 2;

            uint32_t addr = er[b1 >> 4];

            if(!handleBitFlags(b2, addr, b3 >> 4))
                return 0;

            return fetchTiming(2) + byteAccessTiming(addr);
        }

        case 0x7D:
        {
            auto b2 = readByte(pc);
            auto b3 = readByte(pc + 1);
            pc += 2;

            auto addr = er[b1 >> 4];

            if(!handleBitValue(b2, addr, b3 >> 4))
                return 0;

            return fetchTiming(2) + byteAccessTiming(addr) * 2;
        }

        case 0x7E: // bit instructions with 8 bit addresses
        {
            auto b2 = readByte(pc);
            auto b3 = readByte(pc + 1);
            pc += 2;

            uint32_t addr = b1 | 0xFFFF00;

            if(!handleBitFlags(b2, addr, b3 >> 4))
                return 0;

            return fetchTiming(2) + byteAccessTiming(addr);
        }

        case 0x7F: // bit instructions with 8 bit addresses
        {
            auto b2 = readByte(pc);
            auto b3 = readByte(pc + 1);
            pc += 2;

            uint32_t addr = b1 | 0xFFFF00;

            if(!handleBitValue(b2, addr, b3 >> 4))
                return 0;

            return fetchTiming(2) + byteAccessTiming(addr) * 2;
        }
        // 8x-Fx handled above

        default:
            return 0;
    }

    // not reachable
    assert(false);

    return 1;
}

void H8CPU::dumpRegs() const
{
    for(int i = 0; i < 8; i++)
        std::cout << "ER" << i << ": " << std::hex << er[i] << std::dec << " ";

    std::cout << "CCR: " << std::hex << static_cast<int>(ccr) << std::dec << " ";
    std::cout << "PC: " << std::hex << pc << std::dec << std::endl;
}

void H8CPU::setExternalArea(int area, ExternalAddressDevice *device)
{
    externalAreas[area] = device;

    if(area != 0 && area != 7) // first/last areas overlap internal ram/rom
        areaPtrs[area] = device->getPtr(areaMasks[area]);
}

void H8CPU::addIODevice(::IOPort port, IODevice *device)
{
    ioPorts[static_cast<int>(port)].addDevice(device);
}

void H8CPU::setSerialDevice(int index, SerialDevice *device)
{
    serialPorts[index].setDevice(device);
}

void H8CPU::setADCValue(unsigned channel, uint16_t value)
{
    if(channel < 8)
        a2dValues[channel] = value;
}

bool H8CPU::getSleeping() const
{
    return sleeping;
}

uint32_t H8CPU::getClock() const
{
    return clock;
}

uint8_t H8CPU::DMAChannel::getReg(int addr) const
{
    switch(addr)
    {
        case 0x0: // MARA
            return memAddrA >> 24;
        case 0x1:
            return memAddrA >> 16;
        case 0x2:
            return memAddrA >> 8;
        case 0x3:
            return memAddrA & 0xFF;
        case 0x4: // IOARA
            return ioAddrA >> 8;
        case 0x5:
            return ioAddrA & 0xFF;
        case 0x6: // ETCRA
            return countA >> 8;
        case 0x7:
            return countA & 0xFF;
        case 0x8: // MARB
            return memAddrB >> 24;
        case 0x9:
            return memAddrB >> 16;
        case 0xA:
            return memAddrB >> 8;
        case 0xB:
            return memAddrB & 0xFF;
        case 0xC: // IOARB
            return ioAddrB >> 8;
        case 0xD:
            return ioAddrB & 0xFF;
        case 0xE: // ETCRB
            return countB >> 8;
        case 0xF:
            return countB & 0xFF;

        case 0x10: // DMACR
            return control >> 8;
        case 0x11:
            return control & 0xFF;

        case 0x12: // DMABCR (shared)
            return bandControl >> 8;
        case 0x13:
            return bandControl & 0xFF;
    }

    return 0;
}

void H8CPU::DMAChannel::setReg(int addr, uint8_t val)
{
    auto getFullMode = [this](){return bandControl & (1 << (14 + channel));};

    switch(addr)
    {
        case 0x0: // MARA
            memAddrA = (val << 24) | (memAddrA & 0x00FFFFFF);
            break;
        case 0x1:
            memAddrA = (val << 16) | (memAddrA & 0xFF00FFFF);
            break;
        case 0x2:
            memAddrA = (val << 8) | (memAddrA & 0xFFFF00FF);
            break;
        case 0x3:
            memAddrA = val | (memAddrA & 0xFFFFFF00);
            break;
        case 0x4: // IOARA
            ioAddrA = (val << 8) | (ioAddrA & 0x00FF);
            break;
        case 0x5:
            ioAddrA = val | (ioAddrA & 0xFF00);
            break;
        case 0x6: // ETCRA
            countA = (val << 8) | (countA & 0x00FF);
            break;
        case 0x7:
            countA = val | (countA & 0xFF00);
            break;
        case 0x8: // MARB
            memAddrB = (val << 24) | (memAddrB & 0x00FFFFFF);
            break;
        case 0x9:
            memAddrB = (val << 16) | (memAddrB & 0xFF00FFFF);
            break;
        case 0xA:
            memAddrB = (val << 8) | (memAddrB & 0xFFFF00FF);
            break;
        case 0xB:
            memAddrB = val | (memAddrB & 0xFFFFFF00);
            break;
        case 0xC: // IOARB
            ioAddrB = (val << 8) | (ioAddrB & 0x00FF);
            break;
        case 0xD:
            ioAddrB = val | (ioAddrB & 0xFF00);
            break;
        case 0xE: // ETCRB
            countB = (val << 8) | (countB & 0x00FF);
            break;
        case 0xF:
            countB = val | (countB & 0xFF00);
            break;

        case 0x10: // DMACR
            control = (val << 8) | (control & 0x00FF);
            if(getFullMode() && (control & (1 << 11))) // blke
                std::cout << "DMACR" << channel << " BLKE\n";
            break;
        case 0x11:
            control = val | (control & 0xFF00);
            if(getFullMode() && (control & 0xE) != 0x6) // dtf
                std::cout << "DMACR" << channel << " DTF " << std::hex << (control & 0xF) << std::dec << "\n";
            break;

        case 0x12: // DMABCR (shared)
            bandControl = (val << 8) | (bandControl & 0x00FF);
            break;
        case 0x13:
            bandControl = val | (bandControl & 0xFF00);
            break;
    }
}

int H8CPU::DMAChannel::transfer(H8CPU &cpu)
{
    bool fullAddr = bandControl & (1 << (14 + channel));

    int cycles;

    if(fullAddr)
    {
        auto enableMask = 0x30 << (2 * channel);
        // check enabled
        if((bandControl & enableMask) != enableMask)
            return 0;

        // not auto mode
        // TODO
        if((control & 0xE) != 0x6)
            return 0;

        // not block mode
        // TODO
        if(control & (1 << 11)) // blke
            return 0;

        auto size = (control & (1 << 15)) ? 2 : 1;

        // transfer
        if(size == 2)
        {
            cycles = cpu.wordAccessTiming(memAddrA) + cpu.wordAccessTiming(memAddrB);
            cpu.writeWord(memAddrB, cpu.readWord(memAddrA));
        }
        else
        {
            cycles = cpu.byteAccessTiming(memAddrA) + cpu.byteAccessTiming(memAddrB);
            cpu.writeByte(memAddrB, cpu.readByte(memAddrA));
        }

        countA--;

        // clear DTE
        if(countA == 0)
            bandControl &= ~(1 << (4 + channel * 2));

        if(control & (1 << 5)) // daide
        {
            if(control & (1 << 6))
                memAddrB -= size;
            else
                memAddrB += size;
        }

        if(control & (1 << 13)) // saide
        {
            if(control & (1 << 14))
                memAddrA -= size;
            else
                memAddrA += size;
        }
    }
    else
    {
        // TODO: single address
        if(bandControl & 1 << (12 + channel))
            return 0;

        cycles = 0;

        auto doSubChannel = [this, &cpu, &cycles](uint8_t chanControl, int channelShift, uint32_t &memAddr, uint16_t ioAddr, uint16_t &count, InterruptSource interrupt)
        {
            if(!(bandControl & (0x10 << channelShift))) // enabled
                return;

            // someone isn't clearing DTME before FAE...
            if(count == 0)
            {
                bandControl &= ~(0x10 << channelShift);
                return;
            }

            bool repEnable = chanControl & (1 << 5);
            bool endInterruptEnable = bandControl & (1 << channelShift);

            // TODO: repeat mode
            if(repEnable && !endInterruptEnable)
                return;

            auto size = (chanControl & (1 << 7)) ? 2 : 1;

            // activation source
            bool clear = bandControl & (0x100 << channelShift); //DTAxx
            int intrBit, intrIndex;

            switch(chanControl & 0xF)
            {
                /*
                case 1: //ADI
                case 2: //DREQ pin falling edge (B only)
                case 3: //DREQ pin low (B only)
                case 4: //TXI0
                case 5: //RXI0*/
                case 6:
                {
                    intrBit = static_cast<int>(InterruptSource::TXI1) - 64;
                    intrIndex = 1;

                    break;
                }
                case 7:
                {
                    intrBit = static_cast<int>(InterruptSource::RXI1) - 64;
                    intrIndex = 1;

                    break;
                }
                /*case 8: //TGI0A
                case 9: //TGI1A
                case 10: //TGI2A
                case 11: //TGI3A
                case 12: //TGI4A
                case 13: //TGI5A
                */

                default:
                    std::cout << "unhandled DMA trigger " << (chanControl & 0xF) << "\n";
                    bandControl &= ~(0x10 << channelShift);
                    return;
            }

            if(!(cpu.requestedInterrupts[intrIndex] & (1ull << intrBit)))
                return;

            if(clear)
                cpu.requestedInterrupts[intrIndex] &= ~(1ull << intrBit);

            uint32_t srcAddr, dstAddr;

            if(chanControl & (1 << 4)) // DTDIR
            {
                srcAddr = ioAddr | 0xFF0000;
                dstAddr = memAddr;
            }
            else
            {
                srcAddr = memAddr;
                dstAddr = ioAddr | 0xFF0000;
            }

            // transfer
            if(size == 2)
            {
                cycles += cpu.wordAccessTiming(srcAddr) + cpu.wordAccessTiming(dstAddr);
                cpu.writeWord(dstAddr, cpu.readWord(srcAddr));
            }
            else
            {
                cycles += cpu.byteAccessTiming(srcAddr) + cpu.byteAccessTiming(dstAddr);
                cpu.writeByte(dstAddr, cpu.readByte(srcAddr));
            }

            count--;

            if(count == 0)
            {
                // end, clear DTE
                bandControl &= ~(0x10 << channelShift);

                // end interrupt
                if(endInterruptEnable)
                    cpu.interrupt(interrupt);
            }

            if(repEnable && !endInterruptEnable)
            {} // TODO: repeat mode
            else if(!repEnable) // sequential mode
            {
                if(chanControl & (1 << 6)) // DTID
                    memAddr--;
                else
                    memAddr++;
            }
            // else idle mode, don't modify address

            // if this was a write to a tx reg from a tx interrupt, we need to clear TDRE
            // TODO: 0
            if(dstAddr == 0xffff83/*TDR1*/ && (chanControl & 0xF) == 6)
                cpu.serialPorts[1].setReg(4/*status*/, cpu.serialPorts[1].getReg(4) & ~SSR_TDRE, cpu);

            // same for rx
            if(srcAddr == 0xffff85/*RDR1*/ && (chanControl & 0xF) == 7)
                cpu.serialPorts[1].setReg(4/*status*/, cpu.serialPorts[1].getReg(4) & ~SSR_RDRF, cpu);
        };

        // chan A
        doSubChannel(control >> 8, channel * 2, memAddrA, ioAddrA, countA, channel ? InterruptSource::DEND1A : InterruptSource::DEND0A);

        // chan B
        doSubChannel(control & 0xFF, channel * 2 + 1, memAddrB, ioAddrB, countB, channel ? InterruptSource::DEND1B : InterruptSource::DEND0B);
    }

    return cycles;
}

uint8_t H8CPU::IOPort::read() const
{
    uint8_t ret = 0;

    for(auto &dev : devices)
        ret |= dev->read();

    return ret;
}

void H8CPU::IOPort::write(uint8_t val)
{
    for(auto &dev : devices)
        dev->write(val);

    data = val;
}

uint8_t H8CPU::IOPort::getWrittenData() const
{
    return data;
}

void H8CPU::IOPort::setDirection(uint8_t dir)
{
    for(auto &dev : devices)
        dev->setDirection(dir);

    direction = dir;
}

uint8_t H8CPU::IOPort::getDirection() const
{
    return direction;
}

void H8CPU::IOPort::addDevice(IODevice *device)
{
    devices.push_back(device);
}

uint8_t H8CPU::TPU::getReg(int reg) const
{
    switch(reg)
    {
        case 4: // TIER
            return interruptEnable;
        case 5: // TSR
            return status;
        // TODO: implement 16-bit r/w to avoid this?
        case 6: // TCNT
            return counter >> 8;
        case 7:
            return counter & 0xFF;

        case 8: // TGRA H
        case 10: // TGRB H
        case 12: // TGRC H
        case 14: // TGRD H
            return general[(reg >> 1) & 3] >> 8;

        case 9: //TGRA L
        case 11: //TGRB L
        case 13: //TGRC L
        case 15: //TGRD L
            return general[(reg >> 1) & 3] & 0xFF;
    }

    return 0;
}

void H8CPU::TPU::setReg(int reg, uint8_t val)
{
    static const char *regNames[]{
        "TCR",
        "TMDR",
        "TIORH",
        "TIORL",
        /*"TIER",
        "TSR",
        "TCNTH",
        "TCNTL",

        "TGRAH",
        "TGRAL",
        "TGRBH",
        "TGRBL",
        "TGRCH",
        "TGRCL",
        "TGRDH",
        "TGRDL"*/
    };

    switch(reg)
    {
        case 0: // TCR
        {
            // edge bits
            if(val & 0x18)
                std::cout << "TPU" << index << " TCR = " << std::hex << static_cast<int>(val) << std::dec << std::endl;

            // 0: 1, 4, 16, 64, tca,  tcb,  tcc, tcd
            // 1: 1, 4, 16, 64, tca,  tcb,  256, tcnt2 ovr/undr
            // 2: 1, 4, 16, 64, tca,  tcb,  tcc, 1024
            // 3: 1, 4, 16, 64, tca, 1024,  256, 4096
            // 4: 1, 4, 16, 64, tca,  tcc, 1024, TCNT5 ovr/undr
            // 5: 1, 4, 16, 64, tca,  tcc,  256, tcd

            int tpsc = val & 0x7;
            if(tpsc < 4)
                clockShift = tpsc * 2; // 1, 4, 16, 64
            else if(tpsc == 6 && (index == 1 || index == 3 || index == 5))
                clockShift = 8;
            else
            {
                std::cout << "TPU" << index << " tpsc " << tpsc << std::endl;
                clockShift = -1;
            }

            clockDiv = clockShift == -1 ? 0 : 1 << clockShift;

            clearOn = val >> 5;

            if(clearOn == 3 || clearOn == 7)
                std::cout << "TPU" << index << " clear on sync\n";

            calcNextUpdate();

            break;
        }

        case 4: // TIER
            if(val & 0x80)
                std::cout << "TPU" << index << " ADC start\n";
            interruptEnable = val;
            break;

        case 5: // TSR
            status &= val; // can only clear bits
            break;

        case 6: // TCNT H
            counter = (val << 8) | (counter & 0xFF);
            calcNextUpdate();
            break;

        case 7: // TCNT L
            counter = val | (counter & 0xFF00);
            calcNextUpdate();
            break;

        case 8: // TGRA H
        case 10: // TGRB H
        case 12: // TGRC H
        case 14: // TGRD H
            general[(reg >> 1) & 3] = (val << 8) | (general[(reg >> 1) & 3] & 0xFF);
            calcNextUpdate();
            break;

        case 9: //TGRA L
        case 11: //TGRB L
        case 13: //TGRC L
        case 15: //TGRD L
            general[(reg >> 1) & 3] = (val << 8) | (general[(reg >> 1) & 3] & 0xFF);
            calcNextUpdate();
            break;

        default:
            if(index != 1 && index != 2) // sound/vibration
                std::cout << "TPU" << index << " " << regNames[reg] << " = " << std::hex << static_cast<int>(val) << std::dec << std::endl;
    }
}

void H8CPU::TPU::update(H8CPU &cpu)
{
    if(!clockDiv || !(cpu.tpuStart & (1 << index)))
    {
        lastUpdateCycle = cpu.getClock();
        return;
    }

    int elapsed = cpu.getClock() - lastUpdateCycle;
    lastUpdateCycle = cpu.getClock();

    frac += elapsed;

    int inc = frac >> clockShift;

    frac &= clockDiv - 1;

    // skip checking compare/overflow
    int incCounter = static_cast<int>(counter) + inc;
    if(incCounter < nextUpdate)
    {
        counter = incCounter;
        return;
    }

    while(inc)
    {
        int step = std::min(inc, nextUpdate - counter);

        counter += step;
        inc -= step;

        if(counter == 0)
        {
            // overflow
            status |= (1 << 4); // TCFV

            if(interruptEnable & (1 << 4))
            {
                static const InterruptSource interrupts[]
                {
                    InterruptSource::TGI0V,
                    InterruptSource::TGI1V,
                    InterruptSource::TGI2V,
                    InterruptSource::TGI3V,
                    InterruptSource::TGI4V,
                    InterruptSource::TGI5V
                };
                cpu.interrupt(interrupts[index]);
            }
        }

        // TODO: TIOR

        if(counter == general[0])
        {
            // compare match a
            status |= (1 << 0); // TGFA
            if(clearOn == 1)
                counter = 0;

            if(interruptEnable & (1 << 0))
            {
                static const InterruptSource interrupts[]
                {
                    InterruptSource::TGI0A,
                    InterruptSource::TGI1A,
                    InterruptSource::TGI2A,
                    InterruptSource::TGI3A,
                    InterruptSource::TGI4A,
                    InterruptSource::TGI5A
                };
                cpu.interrupt(interrupts[index]);
            }
        }

        if(counter == general[1])
        {
            // compare match b
            status |= (1 << 1); // TGFB
            if(clearOn == 2)
                counter = 0;

            if(interruptEnable & (1 << 1))
            {
                static const InterruptSource interrupts[]
                {
                    InterruptSource::TGI0B,
                    InterruptSource::TGI1B,
                    InterruptSource::TGI2B,
                    InterruptSource::TGI3B,
                    InterruptSource::TGI4B,
                    InterruptSource::TGI5B
                };
                cpu.interrupt(interrupts[index]);
            }
        }

        if(index != 0 && index != 3)
        {
            calcNextUpdate();
            continue;
        }

        if(counter == general[2])
        {
            // compare match c
            status |= (1 << 2); // TGFC
            if(clearOn == 5)
                counter = 0;

            if(interruptEnable & (1 << 2))
                cpu.interrupt(index == 0 ? InterruptSource::TGI0C : InterruptSource::TGI3C);
        }

        if(counter == general[3])
        {
            // compare match d
            status |= (1 << 3); // TGFD
            if(clearOn == 6)
                counter = 0;

            if(interruptEnable & (1 << 3))
                cpu.interrupt(index == 0 ? InterruptSource::TGI0D : InterruptSource::TGI3D);
        }

        calcNextUpdate();
    }
}

void H8CPU::TPU::updateForInterrupts(H8CPU &cpu)
{
    int elapsed = cpu.getClock() - lastUpdateCycle;
    int toNext = nextUpdateCycle - lastUpdateCycle;
    if(interruptEnable && elapsed >= toNext)
        update(cpu);
}

void H8CPU::TPU::calcNextUpdate()
{
    nextUpdate = 0x10000; // overflow

    int numGen = (index == 0 || index == 3) ? 4 : 2;

    // find first match
    for(int i = 0; i < numGen; i++)
    {
        if(general[i] > counter && general[i] < nextUpdate)
            nextUpdate = general[i];
    }

    nextUpdateCycle = lastUpdateCycle + nextUpdate * clockDiv - frac;
}

uint8_t H8CPU::Timer::getReg(int reg) const
{
    switch(reg)
    {
        case 0: // TCR
            return control;
        case 1: // TCSR
            return controlStatus;
        case 2: // TCORA
            return constantA;
        case 3: // TCORB
            return constantB;
        case 4: // TCNT
            return counter;
    }
    return 0;
}

void H8CPU::Timer::setReg(int reg, uint8_t val)
{
    switch(reg)
    {
        case 0: // TCR
        {
            control = val;

            int clockSel = control & 0x7;

            if(clockSel == 1)
                clockDiv = 8;
            else if(clockSel == 2)
                clockDiv = 64;
            else if(clockSel == 3)
                clockDiv = 8192;
            else // external/overflow/compare match (unimplemented)
                clockDiv = 0;

            if((val & 0xD8) == 0 && clockDiv != 0)
                return;
            break;
        }
        case 1: // TCSR
            controlStatus = val;
            if(val == 0)
                return;
            break;
        case 2: // TCORA
            constantA = val;
            break;
        case 3: // TCORB
            constantB = val;
            break;
        case 4: // TCNT
            counter = val;
            return;
    }

    std::cout << "Timer" << index << " reg " << reg << " = " << std::hex << static_cast<int>(val) << "\n" << std::dec;
}

void H8CPU::Timer::update(H8CPU &cpu, int clocks)
{
    if(!clockDiv)
        return;

    // TODO: control bits 3-7

    frac += clocks;

    if(frac < clockDiv)
        return;

    frac -= clockDiv;
    counter++;

    if(counter == 0)
    {
        controlStatus |= 1 << 5; // overflow
        if(control & 0x20)
            cpu.interrupt(index == 0 ? InterruptSource::OVI0 : InterruptSource::OVI1); // there are only two of these
    }

    //if(frac >= clockDiv)
    //    std::cout << "timer MULTI " << index << " " << frac << "/" << clockDiv << "\n";

    // TODO: compare match
    // TODO: p26 output (OS0-4)
}

uint8_t H8CPU::Serial::getReg(int addr) const
{
    switch(addr)
    {
        case 0:
            return mode;
        case 1:
            return bitRate;
        case 2:
            return control;
        case 3:
            return txData;
        case 4:
            return status;
        case 5:
            return rxData;
        case 6:
            return smartCardMode;
    }

    return 0;
}

void H8CPU::Serial::setReg(int addr, uint8_t val, H8CPU &cpu)
{
    switch(addr)
    {
        case 0:
            mode = val;
            break;
        case 1:
            bitRate = val;
            break;
        case 2:

            if(!(control & SCR_TIE) && (val & SCR_TIE) && (val & SCR_TE) && (status & SSR_TDRE))
                forceTXI = true;

            control = val;
            break;
        case 3: // TDR
            txData = val;
            break;
        case 4:
            // top 5 bits can only be cleared, next 2 are read-only
            //status = ((val & status) & 0xF8) | (status & 0x6) | (val & 1);
            status = val;
            break;
        //case 5: // RDR - read only
        case 6:
            smartCardMode = val;
            break;
    }

    if(!(control & SCR_TE))
        status |= SSR_TDRE | SSR_TEND; // set to 1 if disabled
}

void H8CPU::Serial::update(H8CPU &cpu)
{
    // does not handle baud rates...

    if(forceTXI)
    {
        // generate a TXI interrupt on enable
        // otherwise the DMAC won't get triggered
        cpu.interrupt(static_cast<InterruptSource>(static_cast<int>(InterruptSource::TXI0) + index * 4));
        forceTXI = false;
    }

    if(mode & SMR_CA)
    {
        // sync
        bool didTX = false;

        // send
        if(control & SCR_TE)
        {
            if(status & SSR_TDRE)
            {
                if(!(status & SSR_TEND))
                {
                    status |= SSR_TEND;

                    if(control & SCR_TEIE)
                        cpu.interrupt(static_cast<InterruptSource>(static_cast<int>(InterruptSource::TEI0) + index * 4));
                }

                // can't rx if we didn't tx (unless tx is disabled)
                return;

            }
            else
            {
                if(!device)
                    std::cout << "SCI" << index << " write " << std::hex << static_cast<int>(txData) << " cr " << static_cast<int>(control) << " sr " << static_cast<int>(status) << std::dec << "\n";
                else
                    device->write(txData);

                status |= SSR_TDRE;

                if(control & SCR_TIE)
                    cpu.interrupt(static_cast<InterruptSource>(static_cast<int>(InterruptSource::TXI0) + index * 4));

                // if sync also read
                didTX = true;
            }
        }

        // recv
        if(control & SCR_RE)
        {
            if(status & SSR_RDRF)
            {
                // sync overrun
                // TODO: ERI interrupt
                if(didTX)
                    status |= SSR_ORER;
            }
            else
            {
                // get data
                bool gotData = false;

                if(device)
                {
                    // handle sync rx with tx disabled (probably sends some junk?)
                    if(!(control & SCR_TE))
                        device->write(0xFF); // ?

                    if(device->canRead())
                    {
                        rxData = device->read();
                        gotData = true;
                    }
                }

                // set flag if we got something
                // or if we sent something, as we can't not recv when sending
                if(gotData || didTX)
                {
                    status |= SSR_RDRF;
                    if((control & SCR_RIE))
                        cpu.interrupt(static_cast<InterruptSource>(static_cast<int>(InterruptSource::RXI0) + index * 4));
                }
            }
        }
    }
    else
    {
        // send
        if(control & SCR_TE)
        {
            if(status & SSR_TDRE)
            {
                if(!(status & SSR_TEND))
                {
                    status |= SSR_TEND;

                    if(control & SCR_TEIE)
                        cpu.interrupt(static_cast<InterruptSource>(static_cast<int>(InterruptSource::TEI0) + index * 4));
                }
            }
            else
            {
                if(!device)
                    std::cout << "SCI" << index << " write " << std::hex << static_cast<int>(txData) << " cr " << static_cast<int>(control) << " sr " << static_cast<int>(status) << std::dec << "\n";
                else
                    device->write(txData);

                status |= SSR_TDRE;

                if(control & SCR_TIE)
                    cpu.interrupt(static_cast<InterruptSource>(static_cast<int>(InterruptSource::TXI0) + index * 4));
            }
        }

        // recv
        if(control & SCR_RE)
        {
            if(!(status & SSR_RDRF))
            {
                // get data
                if(device && device->canRead())
                {
                    rxData = device->read();

                    // set flag if we got something
                    status |= SSR_RDRF;
                    if((control & SCR_RIE))
                        cpu.interrupt(static_cast<InterruptSource>(static_cast<int>(InterruptSource::RXI0) + index * 4));
                }
            }
        }
    }
}

void H8CPU::Serial::setDevice(SerialDevice *device)
{
    this->device = device;
}

void H8CPU::serviceInterrupt()
{
    if(requestedInterrupts[0] == 0 && requestedInterrupts[1] == 0)
        return;

    bool nmi = requestedInterrupts[0] & (1 << static_cast<int>(InterruptSource::NMI));
    if((ccr & CC_I) && !nmi)
        return;

    if(nmi)
    {
        // do nmi
        return;
    }

    auto checkInterruptMask = [this](uint64_t interrupts, int i)
    {
        for(; interrupts; i++, interrupts >>= 1ull)
        {
            if(interrupts & 1ull)
            {
                // TODO: should check if enabled (checked at request time, may have disabled)

                uint32_t addr = readLong(i * 4);

                er[7] -= 4;
                writeLong(er[7], pc | (ccr << 24));
                pc = addr;
                ccr |= CC_I;
                sleeping = false;

                requestedInterrupts[i >> 6] &= ~(1ull << (i & 0x3F));
                return true;
            }
        }

        return false;
    };

    if(!checkInterruptMask(requestedInterrupts[0], 0))
        checkInterruptMask(requestedInterrupts[1], 64);
}

bool H8CPU::updateDTC()
{
    using IS = InterruptSource;
    static const InterruptSource sources[6][8]
    {
        {IS::IRQ7, IS::IRQ6, IS::IRQ5, IS::IRQ4, IS::IRQ3, IS::IRQ2, IS::IRQ1, IS::IRQ0}, // A
        {IS::TGI1B, IS::TGI1A, IS::TGI0D, IS::TGI0C, IS::TGI0B, IS::TGI0A, IS::ADI, IS::Invalid}, // B
        {IS::TGI4B, IS::TGI4A, IS::TGI3D, IS::TGI3C, IS::TGI3B, IS::TGI3A, IS::TGI2B, IS::TGI2A}, // C
        {IS::CMIB1, IS::CMIA1, IS::CMIB0, IS::CMIA0, IS::TGI5B, IS::TGI5A, IS::Invalid, IS::Invalid}, // D
        {IS::TXI1, IS::RXI1, IS::TXI0, IS::RXI0, IS::DEND1B, IS::DEND1A, IS::DEND0B, IS::DEND0A}, // E
        {IS::Invalid, IS::Invalid, IS::Invalid, IS::Invalid, IS::Invalid, IS::Invalid, IS::TXI2, IS::RXI2} //F
    };

    int vecAddr = -1;
    int intrIdx = 0;
    int i, j;
    //
    static bool once[6][8]{false};
    //

    if(!requestedInterrupts[0] && !requestedInterrupts[1])
        return false; // can't be triggered by interrupts if there aren't any

    for(i = 0; i < 6 && vecAddr == -1; i++)
    {
        auto enabled = dtcEnable[i];

        for(j = 0; enabled; j++, enabled >>= 1)
        {
            // invalid bit or not enabled
            if(sources[i][j] == InterruptSource::Invalid || !(enabled & 1))
                continue;

            intrIdx = static_cast<int>(sources[i][j]);

            // interrupt not requested
            int intrBit = (intrIdx & 0x3F);
            if(!(requestedInterrupts[intrIdx >> 6] & (1ull << intrBit)))
                continue;

            // nope, higher priority interrupt needs handled
            if(requestedInterrupts[intrIdx >> 6] & ((1ull << intrBit) - 1))
                return false;

            vecAddr = readWord(0x400 + intrIdx * 2);

            break;
        }
    }

    if(vecAddr == -1)
        return false; // nothing to do

    // has to point to internal RAM
    vecAddr -= 0xDC00;
    bool chain = false, end;

    uint32_t srcAddr, dstAddr;

    do
    {
        const uint8_t modeA = ram[vecAddr];
        srcAddr = (ram[vecAddr + 1] << 16) | (ram[vecAddr + 2] << 8) | ram[vecAddr + 3];
        const uint8_t modeB = ram[vecAddr + 4];
        dstAddr = (ram[vecAddr + 5] << 16) | (ram[vecAddr + 6] << 8) | ram[vecAddr + 7];
        const uint16_t countA = (ram[vecAddr + 8] << 8) | ram[vecAddr + 9];
        //uint16_t countB = (ram[vecAddr + 10] << 8) | ram[vecAddr + 11];

        auto unhandled = [&]()
        {
            if(!once[i - 1][j])
            {
                uint16_t countB = (ram[vecAddr + 10] << 8) | ram[vecAddr + 11];
                std::cout << "DTC triggered " << std::hex << (vecAddr + 0xDC00) << " mode " << (int)modeA << " " << (int)modeB
                        << " src " << srcAddr << " dst " << dstAddr << " count " << countA << " " << countB << std::dec << "\n";
                once[i - 1][j] = true;
            }
        };

        const int transferMode = (modeA & 0xC) >> 2;

        //TODO: chain if count == 0
        if(modeB & (1 << 5))
            unhandled();

        const int size = (modeA & 1) + 1;

        // transfer
        // counter is lower byte only in block/repeat mode
        int count = transferMode == 0 ? countA : (countA & 0xFF);

        do
        {
            if(size == 2)
                writeWord(dstAddr, readWord(srcAddr));
            else
                writeByte(dstAddr, readByte(srcAddr));

            // src addr mode
            if(modeA & (1 << 7))
            {
                if(modeA & (1 << 6))
                    srcAddr -= size;
                else
                    srcAddr += size;
            }

            // dst addr mode
            if(modeA & (1 << 5))
            {
                if(modeA & (1 << 4))
                    dstAddr -= size;
                else
                    dstAddr += size;
            }

            count--;
        }
        while(count && transferMode == 2); // copy entire block in block mode

        // store / reset count
        if(transferMode == 0) // normal mode, store high byte
        {
            ram[vecAddr + 8] = count >> 8;
            end = count == 0;
        }
        else if(count == 0)
        {
            // repeat or block, reload counter
            count = countA >> 8;

            // decrement block counter
            if(transferMode == 2)
            {
                uint16_t countB = (ram[vecAddr + 10] << 8) | ram[vecAddr + 11];
                countB--;
                ram[vecAddr + 10] = countB >> 8;
                ram[vecAddr + 11] = countB & 0xFF;
                end = countB == 0;
            }
            else
                end = false;

            // reset selected address (DTS)
            bool srcBlock = modeA & (1 << 1);
            if(srcBlock && (modeA & (1 << 7)))
            {
                // "reload" src
                if(modeA & (1 << 6))
                    srcAddr += count * size;
                else
                    srcAddr -= count * size;
            }
            else if(!srcBlock && (modeA & (1 << 5)))
            {
                // "reload" dst
                if(modeA & (1 << 4))
                    dstAddr += count * size;
                else
                    dstAddr -= count * size;
            }
        }

        ram[vecAddr + 9] = count;

        // write back addresses if inc/dec set
        if(modeA & (1 << 7))
        {
            ram[vecAddr + 1] = srcAddr >> 16;
            ram[vecAddr + 2] = srcAddr >> 8;
            ram[vecAddr + 3] = srcAddr;
        }

        if(modeA & (1 << 5))
        {
            ram[vecAddr + 5] = dstAddr >> 16;
            ram[vecAddr + 6] = dstAddr >> 8;
            ram[vecAddr + 7] = dstAddr;
        }

        // are we chaining?
        chain = modeB & (1 << 7);

        // chain only when count 0
        // modeB & (1 << 5)

        // clear interrupt or disable
        if(!chain)
        {
            // DISEL or count is 0
            if((modeB & (1 << 6)) || end)
                dtcEnable[i - 1] &= ~(1 << j);
            else
                requestedInterrupts[intrIdx >> 6] &= ~(1ull << (intrIdx & 0x3F));
        }

        vecAddr += 12;
    }
    while(chain);

    if(end) // DISEL?
        return false; // pass through to interrupt handling

    // SCI flag clearing...
    if(dstAddr == 0xFFFF7B/*TDR0*/)
        serialPorts[0].setReg(4/*status*/, serialPorts[0].getReg(4) & ~SSR_TDRE, *this);
    else if(dstAddr == 0xFFFF83/*TDR1*/)
        serialPorts[1].setReg(4/*status*/, serialPorts[1].getReg(4) & ~SSR_TDRE, *this);
    else if(dstAddr == 0xFFFF8B/*TDR2*/)
        serialPorts[2].setReg(4/*status*/, serialPorts[2].getReg(4) & ~SSR_TDRE, *this);

    if(srcAddr == 0xFFFF7D/*RDR0*/)
        serialPorts[0].setReg(4/*status*/, serialPorts[0].getReg(4) & ~SSR_RDRF, *this);
    else if(srcAddr == 0xFFFF85/*RDR1*/)
        serialPorts[1].setReg(4/*status*/, serialPorts[1].getReg(4) & ~SSR_RDRF, *this);
    else if(srcAddr == 0xFFFF8D/*RDR2*/)
        serialPorts[2].setReg(4/*status*/, serialPorts[2].getReg(4) & ~SSR_RDRF, *this);

    // a/d flag clear
    // ?
    if(srcAddr >= 0xFFFF90 && srcAddr < 0xFFFF98)
    {
        a2dControlStatus &= ~(1 << 7); // ADF
        std::cout << "clear adf\n";
    }

    return true;
}

void H8CPU::updateADC(int cycles)
{
    if(!(a2dControlStatus & (1 << 5))) // ADST
        return;

    a2dConvCounter -= cycles;

    if(a2dConvCounter <= 0)
    {
        a2dData[a2dChannel & 3] = a2dValues[a2dChannel] << 6;

        //if(a2dChannel == 3) // cart detect
        //    a2dData[3] = 0; // < 16 is cart 0, 16-27 is cart 1, ... 965-1022 is cart 33, 1023 is no cart

        if((a2dControlStatus & (1 << 4))) // SCAN
        {
            a2dChannel++;
            if(a2dChannel > a2dEndChannel) // all channels completed
            {
                a2dChannel = a2dEndChannel & 4; // start channel is 0 or 4
                a2dControlStatus |= (1 << 7); // ADF

                if(a2dControlStatus & (1 << 6)) // ADIE
                    interrupt(InterruptSource::ADI);
            }
        }
        else // single mode, set full and disable
        {
            a2dControlStatus |= (1 << 7); // ADF
            a2dControlStatus &= ~(1 << 5);

            if(a2dControlStatus & (1 << 6)) // ADIE
                interrupt(InterruptSource::ADI);
        }

        a2dConvCounter += a2dConversionTime;
    }
}

int H8CPU::handleLongMOV0100()
{
    uint8_t b2 = readByte(pc);
    uint8_t b3 = readByte(pc + 1);
    pc += 2;

    if(b2 == 0x69)
    {
        auto regA = (b3 & 0x70) >> 4;
        auto regB = b3 & 0xF;

        if(b3 & 0x80) //mov.l reg @reg
            writeLong(er[regA], doMOV(er[regB]));
        else //mov.l @reg reg
            er[regB] = doMOV(readLong(er[regA]));

        return fetchTiming(2) + wordAccessTiming(er[regA]) * 2;
    }
    else if(b2 == 0x6B)
    {
        auto reg = b3 & 0xF;
        switch(b3 >> 4)
        {
            case 0: //mov.l @addr:16 reg
            {
                auto addr = readAddr16(pc);
                er[reg] = doMOV(readLong(addr));
                pc += 2;
                return fetchTiming(3) + wordAccessTiming(addr) * 2;
            }

            case 2: //mov.l @addr:32 reg
            {
                auto addr = readLong(pc);
                er[reg] = doMOV(readLong(addr));
                pc += 4;
                return fetchTiming(4) + wordAccessTiming(addr) * 2;
            }

            case 8: //mov.l reg @addr:16
            {
                auto addr = readAddr16(pc);
                writeLong(addr, doMOV(er[reg]));
                pc += 2;
                return fetchTiming(3) + wordAccessTiming(addr) * 2;
            }

            case 0xA: //mov.l reg, @addr:32
            {
                auto addr = readLong(pc);
                writeLong(addr, doMOV(er[reg]));
                pc += 4;
                return fetchTiming(4) + wordAccessTiming(addr) * 2;
            }
        }
    }
    else if(b2 == 0x6D)
    {
        auto regA = (b3 & 0x70) >> 4;
        auto regB = b3 & 0xF;
        if(b3 & 0x80) //mov.l reg @-reg
        {
            er[regA] -= 4;
            writeLong(er[regA], doMOV(er[regB]));
        }
        else //mov.l @reg+ reg
        {
            auto val = doMOV(readLong(er[regA]));
            er[regA] += 4;
            er[regB] = val;
        }

        return fetchTiming(2) + wordAccessTiming(er[regA]) * 2 + 1;
    }
    else if(b2 == 0x6F)
    {
        auto regA = (b3 & 0x70) >> 4;
        auto regB = b3 & 0xF;
        auto addr = er[regA] + readDisp16(pc);

        if(b3 & 0x80) //mov.l reg @reg+off:16
            writeLong(addr, doMOV(er[regB]));
        else //mov.l @reg+off:16 reg
            er[regB] = doMOV(readLong(addr));

        pc += 2;
        return fetchTiming(3) + wordAccessTiming(addr) * 2;
    }
    else if(b2 == 0x78 && readByte(pc) == 0x6B)
    {
        auto b5 = readByte(pc + 1);
        auto regA = (b3 & 0x70) >> 4;
        auto regB = b5 & 0xF;

        // b5 & 0x20 == 0x20?

        switch(b5 >> 4)
        {
            case 0x2: //mov.l reg+addr:32 reg
            {
                auto addr = er[regA] + readLong(pc + 2);
                er[regB] = doMOV(readLong(addr));
                pc += 6;
                return fetchTiming(5) + wordAccessTiming(addr) * 2;
            }
            case 0xA: //mov.l reg reg+addr:32
            {
                auto addr = er[regA] + readLong(pc + 2);
                writeLong(addr, doMOV(er[regB]));
                pc += 6;
                return fetchTiming(5) + wordAccessTiming(addr) * 2;
            }
        }
    }

    std::cerr << "Unhandled 01 00 " << std::hex << static_cast<int>(b2) << " " << static_cast<int>(b3) << "@" << (pc - 4) << std::dec << std::endl;
    return 0;
}

int H8CPU::handleWordLDCSTC()
{
    auto b2 = readByte(pc);
    auto b3 = readByte(pc + 1);

    assert((b3 & 0xF) == 0);

    uint32_t addr = 0;
    int timing = 0;
    bool isStore;

    switch(b2)
    {
        case 0x69: // @reg
        {
            int reg = (b3 >> 4) & 7;
            addr = er[reg];
            timing = fetchTiming(2);
            isStore = b3 & 0x80;
            pc += 2;
            break;
        }

        case 0x6B: // @addr
        {
            if((b3 & 0x7F) == 0) // 16
            {
                addr = readAddr16(pc + 2);
                timing = fetchTiming(3);
                pc += 4;
            }
            else if((b3 & 0x7F) == 0x20) // 32
            {
                addr = readLong(pc + 2);
                timing = fetchTiming(4);
                pc += 6;
            }
            else
                assert(!"Invalid opcode!");

            isStore = b3 & 0x80;

            break;
        }

        case 0x6D: // @reg+/-
        {
            int reg = (b3 >> 4) & 7;
            addr = er[reg];
            timing = fetchTiming(2);
            isStore = b3 & 0x80;
            pc += 2;

            if(isStore)
                er[reg] = addr = addr - 2; // pre-decrement
            else
                er[reg] += 2;

            break;
        }

        case 0x6F: // @reg+16
        {
            int reg = (b3 >> 4) & 7;
            addr = er[reg] + readDisp16(pc + 2);
            timing = fetchTiming(3);
            isStore = b3 & 0x80;
            pc += 4;

            break;
        }

        case 0x78: // @reg+32
        {
            int reg = (b3 >> 4) & 7;
            auto b5 = readByte(pc + 3);
            addr = er[reg] + readLong(pc + 4);

            assert(readByte(pc + 2) == 0x6B);
            assert((b5 & 0x7F) == 0x20);

            timing = fetchTiming(5);
            isStore = b5 & 0x80;

            pc += 8;

            break;
        }

        default:
            assert(!"Invalid opcode!");
            return 0;
    }

    if(isStore) // stc
        writeWord(addr, (ccr << 8) | ccr); // low byte is undefined, but seems to be the same
    else
        ccr = readWord(addr) >> 8;

    return timing + wordAccessTiming(addr);
}

int H8CPU::handleLong01F0()
{
    uint8_t b2 = readByte(pc);
    uint8_t b3 = readByte(pc + 1);
    pc += 2;

    switch(b2)
    {
        case 0x64: //or.l reg reg
            doOR(er[b3 >> 4], er[b3 & 0xF]);
            break;

        case 0x65: //xor.l reg reg
            doXOR(er[b3 >> 4], er[b3 & 0xF]);
            break;

        case 0x66: //and.l reg reg
            doAND(er[b3 >> 4], er[b3 & 0xF]);
            break;

        default:
            assert(false);
            return 0; // shouldn't happen
    }

    return fetchTiming(2);
}

int H8CPU::handleADDSINC(uint8_t b1)
{
    switch(b1 >> 4)
    {
        case 0: //adds 1
            er[b1 & 0xF] += 1;
            return fetchTiming(1);
        // 1-4 invalid
        case 5: //inc.w 1
            doINC(reg<W>(b1 & 0xF));
            return fetchTiming(1);
        // 6 invalid
        case 7: // inc.l 1
            doINC(reg<L>(b1 & 0x7));
            return fetchTiming(1);
        case 8: //adds 2
            er[b1 & 0xF] += 2;
            return fetchTiming(1);
        case 9: //adds 4
            er[b1 & 0xF] += 4;
            return fetchTiming(1);
        // a-c invalid
        case 0xD: // inc.w 2
            doINC<W, 2>(reg<W>(b1 & 0xF));
            return fetchTiming(1);
        // e invalid
        case 0xF: // inc.l 2
            doINC<L, 2>(reg<L>(b1 & 0xF));
            return fetchTiming(1);
    }

    assert(!"Invalid opcode!");

    return 0;
}

int H8CPU::handleLeftShift(uint8_t b1)
{
    auto regIdx = b1 & 0xF;
    switch(b1 >> 4)
    {
        case 0x0: //shll.b
            doSHLL(reg<B>(regIdx));
            break;
        case 0x1: //shll.w
            doSHLL(reg<W>(regIdx));
            break;
        // 2 invalid

        case 0x3: //shll.l
            doSHLL(reg<L>(regIdx));
            break;
        case 0x4: //shll.b 2
            doSHLL<B, 2>(reg<B>(regIdx));
            break;
        case 0x5: //shll.w 2
            doSHLL<W, 2>(reg<W>(regIdx));
            break;

        // 6 invalid

        case 0x7: //shll.l 2
            doSHLL<L, 2>(reg<L>(regIdx));
            break;

        case 0x8: // shal.b 1
            doSHAL(reg<B>(regIdx));
            break;
        case 0x9: //shal.w 1
            doSHAL(reg<W>(regIdx));
            break;

        // A invalid

        case 0xB: //shal.l 1
            doSHAL(reg<L>(regIdx));
            break;
        case 0xC: // shal.b 2
            doSHAL<B, 2>(reg<B>(regIdx));
            break;
        case 0xD: // shal.w 2
            doSHAL<W, 2>(reg<W>(regIdx));
            break;

        // E invalid

        case 0xF: //shal.l 2
            doSHAL<L, 2>(reg<L>(regIdx));
            break;

        default:
            assert(!"Invalid opcode!");
            return 0;
    }

    return fetchTiming(1);
}

int H8CPU::handleRightShift(uint8_t b1)
{
    auto regIdx = b1 & 0xF;
    switch(b1 >> 4)
    {
        case 0x0: //shlr.b
            doSHLR(reg<B>(regIdx));
            break;
        case 0x1: //shlr.w
            doSHLR(reg<W>(regIdx));
            break;
        // 2 invalid
        case 0x3: //shlr.l
            doSHLR(reg<L>(regIdx));
            break;
        case 0x4: //shlr.b 2
            doSHLR<B, 2>(reg<B>(regIdx));
            break;
        case 0x5: //shlr.w 2
            doSHLR<W, 2>(reg<W>(regIdx));
            break;
        // 6 invalid
        case 0x7: //shlr.l 2
            doSHLR<L, 2>(reg<L>(regIdx));
            break;
        case 0x8: // shar.b 1
            doSHAR(reg<B>(regIdx));
            break;
        case 0x9: //shar.w
            doSHAR(reg<W>(regIdx));
            break;
        // A invalid
        case 0xB: //shar.l
            doSHAR(reg<L>(regIdx));
            break;
        case 0xC: // shar.b 2
            doSHAR<B, 2>(reg<B>(regIdx));
            break;
        case 0xD: //shar.w 2
            doSHAR<W, 2>(reg<W>(regIdx));
            break;
        // E invalid
        case 0xF: //shar.l 2
            doSHAR<L, 2>(reg<L>(regIdx));
            break;

        default:
            assert(!"Invalid opcode!");
            return 0;
    }

    return fetchTiming(1);
}

int H8CPU::handleLeftRotate(uint8_t b1)
{
    auto regIdx = b1 & 0xF;

    switch(b1 >> 4)
    {
        case 0x0: //rotxl.b
            doROTXL(reg<B>(regIdx));
            break;
        case 0x1: //rotxl.w
            doROTXL(reg<W>(regIdx));
            break;
        // 2 invalid

        case 0x3: //rotxl.l
            doROTXL(reg<L>(regIdx));
            break;
        case 0x4: //rotxl.b 2
            doROTXL<B, 2>(reg<B>(regIdx));
            break;
        case 0x5: //rotxl.w 2
            doROTXL<W, 2>(reg<W>(regIdx));
            break;
        // 6 invalid
        case 0x7: //rotxl.l 2
            doROTXL<L, 2>(reg<L>(regIdx));
            break;
        case 0x8: //rotl.b
            doROTL(reg<B>(regIdx));
            break;
        case 0x9: //rotl.w
            doROTL(reg<W>(regIdx));
            break;

        // A invalid

        case 0xB: //rotl.l
            doROTL(reg<L>(regIdx));
            break;
        case 0xC: // rotl.b 2
            doROTL<B, 2>(reg<B>(regIdx));
            break;
        case 0xD: //rotl.w 2
            doROTL<W, 2>(reg<W>(regIdx));
            break;
        // E invalid
        case 0xF: //rotl.l 2
            doROTL<L, 2>(reg<L>(regIdx));
            break;

        default:
            assert(!"Invalid opcode!");
            return 0;
    }

    return fetchTiming(1);
}

int H8CPU::handleRightRotate(uint8_t b1)
{
    auto regIdx = b1 & 0xF;

    switch(b1 >> 4)
    {
        case 0x0: //rotxr.b
            doROTXR(reg<B>(regIdx));
            break;
        case 0x1: //rotxr.w
            doROTXR(reg<W>(regIdx));
            break;
        // 2 invalid
        case 0x3: //rotxr.l
            doROTXR(reg<L>(regIdx));
            break;
        case 0x4: //rotxr.b 2
            doROTXR<B, 2>(reg<B>(regIdx));
            break;
        case 0x5: //rotxr.w 2
            doROTXR<W, 2>(reg<W>(regIdx));
            break;
        // 6 invalid
        case 0x7: //rotxr.l 2
            doROTXR<L, 2>(reg<L>(regIdx));
            break;
        case 0x8: //rotr.b
            doROTR(reg<B>(regIdx));
            break;
        case 0x9: //rotr.w
            doROTR(reg<W>(regIdx));
            break;
        // A invalid
        case 0xB: //rotr.l
            doROTR(reg<L>(regIdx));
            break;
        case 0xC: // rotr.b 2
            doROTR<B, 2>(reg<B>(regIdx));
            break;
        case 0xD: //rotr.w 2
            doROTR<W, 2>(reg<W>(regIdx));
            break;
        // E invalid
        case 0xF: //rotr.l 2
            doROTR<L, 2>(reg<L>(regIdx));
            break;

        default:
            assert(!"Invalid opcode!");
            return 0;
    }

    return fetchTiming(1);
}

int H8CPU::handleNOTEXTNEG(uint8_t b1)
{
    auto regIdx = b1 & 0xF;
    switch(b1 >> 4)
    {
        case 0x0: //not.b
        {
            uint8_t res = ~reg<B>(regIdx);
            reg<B>(regIdx) = res;
            updateFlags(res);
            updateFlag(CC_V, false);

            return fetchTiming(1);
        }
        case 0x1: //not.w
        {
            uint16_t res = ~reg<W>(regIdx);
            reg<W>(regIdx) = res;
            updateFlags(res);
            updateFlag(CC_V, false);

            return fetchTiming(1);
        }
        case 0x3: //not.l
            er[regIdx] = ~er[regIdx];
            updateFlags(er[regIdx]);
            updateFlag(CC_V, false);

            return fetchTiming(1);
        case 0x5: //extu.w
        {
            uint16_t res = reg<W>(regIdx) & 0xFF;
            reg<W>(regIdx) = res;

            updateFlags(res);
            updateFlag(CC_V, false);

            return fetchTiming(1);
        }
        case 0x7: //extu.l
            er[regIdx] &= 0xFFFF;

            updateFlags(er[regIdx]);
            updateFlag(CC_V, false);

            return fetchTiming(1);
        case 0x8: //neg.b
        {
            auto val = reg<B>(regIdx);
            uint8_t res = 0 - val;
            reg<B>(regIdx) = res;

            updateFlags(res);
            updateFlag(CC_V, res == 0x80);
            updateFlag(CC_C, res > 0);
            updateFlag(CC_H, (res & 0xF) > 0);

            return fetchTiming(1);
        }
        case 0x9: //neg.w
        {
            auto val = reg<W>(regIdx);
            uint16_t res = 0 - val;
            reg<W>(regIdx) = res;

            updateFlags(res);
            updateFlag(CC_V, res == 0x8000);
            updateFlag(CC_C, res > 0);
            updateFlag(CC_H, (res & 0xFFF) > 0);

            return fetchTiming(1);
        }
        case 0xB: //neg.l
        {
            auto val = er[regIdx];
            uint32_t res = 0 - val;
            er[regIdx] = res;

            updateFlags(res);
            updateFlag(CC_V, res == 0x80000000);
            updateFlag(CC_C, res > 0);
            updateFlag(CC_H, (res & 0xFFFFFFF) > 0);

            return fetchTiming(1);
        }
        case 0xD: //exts.w
        {
            uint16_t val = reg<W>(regIdx);
            val = static_cast<int8_t>(val);

            reg<W>(regIdx) = val;
            updateFlags(val);
            updateFlag(CC_V, false);

            return fetchTiming(1);
        }
        case 0xF: //exts.l
            er[regIdx] = static_cast<int16_t>(er[regIdx]);

            updateFlags(er[regIdx]);
            updateFlag(CC_V, false);

            return fetchTiming(1);

        case 0x2:
        case 0x4:
        case 0x6:
        case 0xA:
        case 0xC:
        case 0xE:
            // invalid
            assert(false);
            return 0;
    }

    return 0;
}

int H8CPU::handleSUBSDEC(uint8_t b1)
{
    auto regIdx = b1 & 0xF;
    switch(b1 >> 4)
    {
        case 0: //subs 1
            er[regIdx] -= 1;
            return fetchTiming(1);

        // 1-4 invalid

        case 5: //dec.w 1
            doDEC(reg<W>(b1 & 0xF));
            return fetchTiming(1);

        // 6 invalid

        case 7: //dec.l 1
            doDEC(reg<L>(b1 & 0xF));
            return fetchTiming(1);
        case 8: //subs 2
            er[regIdx] -= 2;
            return fetchTiming(1);
        case 9: //subs 4
            er[regIdx] -= 4;
            return fetchTiming(1);

        // a-c invalid

        case 0xD: //dec.w 2
            doDEC<W, 2>(reg<W>(b1 & 0xF));
            return fetchTiming(1);

        // e invalid

        case 0xF: //dec.l 2
            doDEC<L, 2>(reg<L>(b1 & 0xF));
            return fetchTiming(1);
    }

    assert(!"Invalid opcode!");

    return 0;
}

int H8CPU::handleBranch(uint8_t conditionCode, int offset)
{
    bool condition;
    switch(conditionCode)
    {
        case 0: //bra
            condition = true;
            break;
        case 1: // brn
            condition = false;
            break;
        case 2: //bhi
            condition = !((ccr & CC_C) || (ccr & CC_Z)); // C or Z == 0
            break;
        case 3: //bls
            condition = (ccr & CC_C) || (ccr & CC_Z); // C or Z == 1
            break;
        case 4: //bcc
            condition = !(ccr & CC_C);
            break;
        case 5: //bcs
            condition = ccr & CC_C;
            break;
        case 6: //bne
            condition = !(ccr & CC_Z);
            break;
        case 7: //beq
            condition = ccr & CC_Z;
            break;
        case 8: // bvc
            condition = !(ccr & CC_V);
            break;
        case 9: // bvs
            condition = ccr & CC_V;
            break;
        case 10: // bpl
            condition = !(ccr & CC_N);
            break;
        case 11: // bmi
            condition = ccr & CC_N;
            break;
        case 12: //bge
            condition = (!(ccr & CC_N) != !(ccr & CC_V)) == false; // N xor V == 0
            break;
        case 13: //blt
            condition = !(ccr & CC_N) != !(ccr & CC_V); // N xor V == 1
            break;
        case 14: //bgt
        {
            auto nXorV = !(ccr & CC_N) != !(ccr & CC_V);
            condition = ((ccr & CC_Z) || nXorV) == false; // Z or (N xor V) == 0
            break;
        }
        case 15: //ble
        {
            auto nXorV = !(ccr & CC_N) != !(ccr & CC_V);
            condition = (ccr & CC_Z) || nXorV; // Z or (N xor V) == 1
            break;
        }

        default:
            assert(!"Invalid codition");
            return 0;
    }

    if(condition)
        pc += offset;

    return fetchTiming(2); // all branches have 2 fetch states
}

int H8CPU::handle6A(uint8_t b1)
{
    switch(b1 >> 4)
    {
        case 0x0: //mov.b addr:16 reg
        {
            auto addr = readAddr16(pc);
            reg<B>(b1 & 0xF) = doMOV(readByte(addr));
            pc += 2;
            return fetchTiming(2) + byteAccessTiming(addr);
        }
        case 0x1:
        {
            auto addr = readAddr16(pc);
            auto b4 = readByte(pc + 2);
            auto b5 = readByte(pc + 3);

            pc += 4;

            if(b1 == 0x10) // bit and/or/load/test/xor instructions with 16 bit addresses
            {
                if(!handleBitFlags(b4, addr, b5 >> 4))
                {
                    assert(!"Invalid opcode!");
                    return 0;
                }

                return fetchTiming(3) + byteAccessTiming(addr);
            }
            else if(b1 == 0x18) // bit set/clear/not instructions with 16 bit addresses
            {
                if(!handleBitValue(b4, addr, b5 >> 4))
                {
                    assert(!"Invalid opcode!");
                    return 0;
                }

                return fetchTiming(3) + byteAccessTiming(addr) * 2;
            }

            return 0;
        }
        case 0x2: //mov.b addr:32 reg
        {
            auto addr = readLong(pc);
            reg<B>(b1 & 0xF) = doMOV(readByte(addr));
            pc += 4;
            return fetchTiming(3) + byteAccessTiming(addr);
        }
        case 0x3:
        {
            auto addr = readLong(pc);
            auto b6 = readByte(pc + 4);
            auto b7 = readByte(pc + 5);

            pc += 6;

            if(b1 == 0x30) // bit and/or/load/test/xor instructions with 32 bit addresses
            {
                if(!handleBitFlags(b6, addr, b7 >> 4))
                {
                    assert(!"Invalid opcode!");
                    return 0;
                }

                return fetchTiming(4) + byteAccessTiming(addr);
            }
            else if(b1 == 0x38) // bit set/clear/not instructions with 32 bit addresses
            {
                if(!handleBitValue(b6, addr, b7 >> 4))
                {
                    assert(!"Invalid opcode!");
                    return 0;
                }

                return fetchTiming(4) + byteAccessTiming(addr) * 2;
            }
            return 0;
        }

        // 4 movfpe
        // 5-7 invalid

        case 0x8: //mov.b reg addr:16
        {
            auto addr = readAddr16(pc);
            writeByte(addr, doMOV(reg<B>(b1 & 0xF)));
            pc += 2;
            return fetchTiming(2) + byteAccessTiming(addr);
        }
        // 9 invalid
        case 0xA: //mov.b reg addr:32
        {
            auto addr = readLong(pc);
            writeByte(addr, doMOV(reg<B>(b1 & 0xF)));
            pc += 4;
            return fetchTiming(3) + byteAccessTiming(addr);
        }
        // B invalid
        // C movtpe
        // D-F invalid

        default:
            std::cerr << "Unhandled 6a " << std::hex << static_cast<int>(b1 >> 4) << "@" << (pc - 2) << std::dec << std::endl;
    }

    return 0;
}

int H8CPU::handle6B(uint8_t b1)
{
    auto regIdx= b1 & 0xF;

    switch(b1 >> 4)
    {
        case 0x0: //mov.w addr:16 reg
        {
            auto addr = readAddr16(pc);
            reg<W>(regIdx) = doMOV(readWord(addr));
            pc += 2;
            return fetchTiming(2) + wordAccessTiming(addr);
        }

        case 0x2: //mov.w addr:32 reg
        {
            auto addr = readLong(pc);
            reg<W>(regIdx) = doMOV(readWord(addr));
            pc += 4;
            return fetchTiming(3) + wordAccessTiming(addr);
        }

        case 0x8: //mov.w reg addr:16
        {
            auto addr = readAddr16(pc);
            writeWord(addr, doMOV(reg<W>(regIdx)));
            pc += 2;
            return fetchTiming(2) + wordAccessTiming(addr);
        }

        case 0xA: // mov.w reg addr:32
        {
            auto addr = readLong(pc);
            writeWord(addr, doMOV(reg<W>(regIdx)));
            pc += 4;
            return fetchTiming(3) + wordAccessTiming(addr);
        }

        default:
            // not valid
            assert(!"Invalid opcode!");
            return 0;
    }
}

int H8CPU::handleMOV78(uint8_t b1)
{
    auto b2 = readByte(pc);
    auto b3 = readByte(pc + 1);
    auto off = readLong(pc + 2);
    pc += 6;

    auto regA = b1 >> 4;
    auto regB = b3 & 0xF;

    if(b2 == 0x6A)
    {
        switch(b3 >> 4)
        {
            case 0x2: //mov.b reg+addr:32 reg
                reg<B>(regB) = doMOV(readByte(er[regA] + off));
                return fetchTiming(4) + byteAccessTiming(er[regA] + off);

            case 0xA: // mov.b reg reg+addr:32
                writeByte(er[regA] + off, doMOV(reg<B>(regB)));
                return fetchTiming(4) + byteAccessTiming(er[regA] + off);
        }
    }
    else if(b2 == 0x6B)
    {
        switch(b3 >> 4)
        {
            case 0x2: //mov.w reg+addr:32 reg
                reg<W>(regB) = doMOV(readWord(er[regA] + off));
                return fetchTiming(4) + wordAccessTiming(er[regA] + off);
            case 0xA: //mov.w reg reg+addr:32
                writeWord(er[regA] + off, doMOV(reg<W>(regB)));
                return fetchTiming(4) + wordAccessTiming(er[regA] + off);
        }
    }

    assert(!"Invalid opcode!");
    return 0;
}

int H8CPU::handleWord79(uint8_t b1)
{
    auto regIdx = b1 & 0xF;
    auto imm = readWord(pc);
    pc += 2;

    switch(b1 >> 4)
    {
        case 0x0: //mov
            reg<W>(regIdx) = doMOV(imm);
            break;
        case 0x1: //add
            doADD(imm, reg<W>(regIdx));
            break;
        case 0x2: //cmp
            doCMP(imm, reg<W>(regIdx));
            break;
        case 0x3: //sub
            doSUB(imm, reg<W>(regIdx));
            break;
        case 0x4: //or
            doOR(imm, reg<W>(regIdx));
            break;
        case 0x5: //xor
            doXOR(imm, reg<W>(regIdx));
            break;
        case 0x6: //and
            doAND(imm, reg<W>(regIdx));
            break;

        // 7+: invalid

        default:
            assert(!"Invalid opcode!");
            return 0;
    }

    return fetchTiming(2);
}

int H8CPU::handleLong7A(uint8_t b1)
{
    auto imm = readLong(pc);
    pc += 4;

    switch(b1 >> 4)
    {
        case 0x0: //mov
            er[b1 & 0xF] = doMOV(imm);
            break;
        case 0x1: //add
            doADD(imm, er[b1 & 0xF]);
            break;
        case 0x2: //cmp
            doCMP(imm, er[b1 & 0xF]);
            break;
        case 0x3: //sub
            doSUB(imm, er[b1 & 0xF]);
            break;
        case 0x4: //or
            doOR(imm, er[b1 & 0xF]);
            break;
        case 0x5: //xor
            doXOR(imm, er[b1 & 0xF]);
            break;
        case 0x6: //and
            doAND(imm, er[b1 & 0xF]);
            break;

        // 7+: invalid

        default:
            assert(!"Invalid opcode!");
            return 0;
    }

    return fetchTiming(3);
}

bool H8CPU::handleBitFlags(uint8_t op, uint32_t addr, int bitVal)
{
    switch(op)
    {
        case 0x63: //btst reg
        {
            auto bit = reg<B>(bitVal) & 0x7;
            updateFlag(CC_Z, (readByte(addr) & (1 << bit)) == 0);
            break;
        }
        case 0x73: //btst imm
        {
            auto bit = bitVal;
            updateFlag(CC_Z, (readByte(addr) & (1 << bit)) == 0);
            break;
        }
        case 0x74: //bor/bior imm
        {
            auto bit = bitVal & 7;
            bool c = ccr & CC_C;
            bool v = readByte(addr) & (1 << bit);

            if(bitVal & 8) // bior
                v = !v;

            updateFlag(CC_C, v || c);

            break;
        }
        case 0x75: //bxor/bixor imm
        {
            auto bit = bitVal & 7;
            bool c = ccr & CC_C;
            bool v = readByte(addr) & (1 << bit);

            if(bitVal & 8) // bixor
                v = !v;

            updateFlag(CC_C, v != c);

            break;
        }
        case 0x76: //band/biand imm
        {
            auto bit = bitVal & 7;
            bool c = ccr & CC_C;
            bool v = readByte(addr) & (1 << bit);

            if(bitVal & 8) // biand
                v = !v;

            updateFlag(CC_C, v && c);

            break;
        }
        case 0x77: //bld/bild imm
        {
            auto bit = bitVal & 7;
            if(bitVal & 8) // bild
                updateFlag(CC_C, !(readByte(addr) & (1 << bit)));
            else // bld
                updateFlag(CC_C, readByte(addr) & (1 << bit));
            break;
        }
        default:
            return false;
    }

    return true;
}

bool H8CPU::handleBitValue(uint8_t op, uint32_t addr, int bitVal)
{
    switch(op)
    {
        case 0x60: // bset reg
        {
            auto bit = reg<B>(bitVal) & 7;
            writeByte(addr, readByte(addr) | (1 << bit));
            break;
        }
        case 0x61: // bnot reg
        {
            auto bit = reg<B>(bitVal) & 7;
            writeByte(addr, readByte(addr) ^ (1 << bit));
            break;
        }
        case 0x62: // bclr reg
        {
            auto bit = reg<B>(bitVal) & 7;
            writeByte(addr, readByte(addr) & ~(1 << bit));
            break;
        }
        case 0x67: // bst/bist imm
        {
            auto bit = bitVal & 7;
            bool v = ccr & CC_C;

            if(bitVal & 8) // bist
                v = !v;

            if(v)
                writeByte(addr, readByte(addr) | (1 << bit));
            else
                writeByte(addr, readByte(addr) & ~(1 << bit));

            break;
        }
        case 0x70: // bset imm
        {
            auto bit = bitVal;
            writeByte(addr, readByte(addr) | (1 << bit));
            break;
        }
        case 0x71: // bnot imm
        {
            auto bit = bitVal;
            writeByte(addr, readByte(addr) ^ (1 << bit));
            break;
        }
        case 0x72: // bclr imm
        {
            auto bit = bitVal;
            writeByte(addr, readByte(addr) & ~(1 << bit));
            break;
        }

        default:
            return false;
    }
    return true;
}

template<class T>
void H8CPU::doADD(T a, T &b)
{
    const auto signBit = 1u << (sizeof(T) * 8 - 1); // 7, 15, 31
    const auto hMask = 0xFFFFFFF >> ((3 - (sizeof(T) - 1)) * 8); // FFFFFFF, FFF, F

    T res = a + b;

    updateFlags(res);
    updateFlag(CC_C, res < b);
    updateFlag(CC_V, ~(a ^ b) & (b ^ res) & signBit); // same signs and sign changed
    updateFlag(CC_H, (res & hMask) < (b & hMask));

    b = res;
}

template<class T>
void H8CPU::doADDX(T a, T &b)
{
    const auto signBit = 1u << (sizeof(T) * 8 - 1); // 7, 15, 31
    const auto hMask = 0xFFFFFFF >> ((3 - (sizeof(T) - 1)) * 8); // FFFFFFF, FFF, F

    int c = (ccr & CC_C) ? 1 : 0;

    T res = a + b + c;

    // this does not set the Z flag
    updateFlag(CC_Z, false);
    updateFlag(CC_N, res & signBit);
    updateFlag(CC_C, res < b + c);
    updateFlag(CC_V, ~(a ^ b) & (b ^ res) & signBit); // same signs and sign changed
    updateFlag(CC_H, (res & hMask) < (b & hMask) + c);

    b = res;
}


template<class T>
void H8CPU::doAND(T a, T &b)
{
    b &= a;
    updateFlags(b);
    updateFlag(CC_V, false); // clear v flag
}

template<class T>
void H8CPU::doCMP(T a, T b)
{
    doSUB(a, b);
}

template<class T, int n>
void H8CPU::doDEC(T &r)
{
    const auto overflowVal = 1u << (sizeof(T) * 8 - 1);

    updateFlag(CC_V, r == overflowVal || (n == 2 && r == overflowVal + 1)); //only value(s) that can cause overflow
    r -= n;
    updateFlags(r);
}

template<class T, int n>
void H8CPU::doINC(T &r)
{
    const auto overflowVal = (1u << (sizeof(T) * 8 - 1)) - 1;

    updateFlag(CC_V, r == overflowVal || (n == 2 && r == overflowVal - 1)); //only value(s) that can cause overflow
    r += n;
    updateFlags(r);
}

template<class T>
T H8CPU::doMOV(T val)
{
    updateFlag(CC_V, false); //clear v
    updateFlags(val);
    return val;
}

template<class T>
void H8CPU::doOR(T a, T &b)
{
    b |= a;
    updateFlags(b);
    updateFlag(CC_V, false); // clear v flag
}

template<class T, int s>
void H8CPU::doROTL(T &r)
{
    const auto topBit = 1u << (sizeof(T) * 8 - s); // 7, 15, 31 or 6, 14, 30
    const auto topShift = (sizeof(T) * 8 - s);

    updateFlag(CC_C, r & topBit);
    r = (r << s) | (r >> topShift);
    updateFlag(CC_V, 0);
    updateFlags(r);
}

template<class T, int s>
void H8CPU::doROTR(T &r)
{
    const auto bottomBit = s; // 1 or 2
    const auto topShift = (sizeof(T) * 8 - s);

    updateFlag(CC_C, r & bottomBit);
    r = (r >> s) | (r << topShift);
    updateFlag(CC_V, 0);
    updateFlags(r);
}

template<class T, int s>
void H8CPU::doROTXL(T &r)
{
    const auto topBit = 1u << (sizeof(T) * 8 - s); // 7, 15, 31 or 6, 14, 30
    const auto bottomBit = s; // 1 or 2
    const auto topShift = (sizeof(T) * 8 - (s - 1));

    bool carry = ccr & CC_C;
    updateFlag(CC_C, r & topBit);

    // avoid warning from bad shift
    if(s == 2)
        r = (r << s) | (r >> topShift);
    else
        r = r << s;

    if(carry)
        r |= bottomBit;

    updateFlag(CC_V, 0);
    updateFlags(r);
}

template<class T, int s>
void H8CPU::doROTXR(T &r)
{
    const auto topBit = 1u << (sizeof(T) * 8 - s); // 7, 15, 31 or 6, 14, 30
    const auto bottomBit = s; // 1 or 2
    const auto topShift = (sizeof(T) * 8 - (s - 1));

    bool carry = ccr & CC_C;
    updateFlag(CC_C, r & bottomBit);

    if(s == 2)
        r = (r >> s) | (r << topShift);
    else
        r = r >> s;

    if(carry)
        r |= topBit;

    updateFlag(CC_V, 0);
    updateFlags(r);
}

template<class T, int s>
void H8CPU::doSHAL(T &r)
{
    const auto signBit = 1u << (sizeof(T) * 8 - 1); // 7, 15, 31
    const auto topBit = 1u << (sizeof(T) * 8 - s); // 7, 15, 31 or 6, 14, 30

    auto val = r;

    r <<= s;

    updateFlags(r);
    updateFlag(CC_C, val & topBit);

    // extra overflow check for 2x shift...
    if(s == 2)
    {
        updateFlag(CC_V, !(val & signBit) != !(r & signBit) ||
                         !(val & topBit) != !(r & signBit));
    }
    else
        updateFlag(CC_V, (val ^ r) & signBit);
}

template<class T, int s>
void H8CPU::doSHAR(T &r)
{
    const auto bottomBit = s; // 1 or 2

    std::make_signed_t<T> val = r;

    r = val >> s;

    updateFlags(r);
    updateFlag(CC_C, val & bottomBit);
    updateFlag(CC_V, 0);
}

template<class T, int s>
void H8CPU::doSHLL(T &r)
{
    const auto topBit = 1u << (sizeof(T) * 8 - s); // 7, 15, 31 or 6, 14, 30

    updateFlag(CC_C, r & topBit);
    r <<= s;
    updateFlag(CC_V, 0);
    updateFlags(r);
}

template<class T, int s>
void H8CPU::doSHLR(T &r)
{
    const auto bottomBit = s; // 1 or 2

    auto val = r;

    r = val >> s;

    updateFlags(r);
    updateFlag(CC_C, val & bottomBit);
    updateFlag(CC_V, 0);
}

template<class T>
void H8CPU::doSUB(T a, T &b)
{
    const auto signBit = 1u << (sizeof(T) * 8 - 1); // 7, 15, 31
    const auto hMask = 0xFFFFFFF >> ((3 - (sizeof(T) - 1)) * 8); // FFFFFFF, FFF, F

    T res = b - a;

    updateFlags(res);
    updateFlag(CC_C, res > b);
    updateFlag(CC_V, ((a ^ b) & signBit) && ((b ^ res) & signBit)); // different signs and sign changed
    updateFlag(CC_H, (res & hMask) > (b & hMask));

    b = res;
}

template<class T>
void H8CPU::doSUBX(T a, T &b)
{
    const auto signBit = 1u << (sizeof(T) * 8 - 1); // 7, 15, 31
    const auto hMask = 0xFFFFFFF >> ((3 - (sizeof(T) - 1)) * 8); // FFFFFFF, FFF, F

    int c = (ccr & CC_C) ? 1 : 0;

    T res = b - a - c;

    // this does not set the Z flag
    updateFlag(CC_Z, false);
    updateFlag(CC_N, res & signBit);
    updateFlag(CC_C, res > b - c);
    updateFlag(CC_V, ((a ^ b) & signBit) && ((b ^ res) & signBit)); // different signs and sign changed
    updateFlag(CC_H, (res & hMask) > (b & hMask) - c);

    b = res;
}

template<class T>
void H8CPU::doXOR(T a, T &b)
{
    b ^= a;
    updateFlags(b);
    updateFlag(CC_V, false); // clear v flag
}

void H8CPU::setPC(uint32_t addr, bool isCall, bool isRet)
{
    pc = addr;
}

// "common" flags
template<class T>
void H8CPU::updateFlags(T val)
{
    auto highBit = 1u << (sizeof(T) * 8 - 1); // 7, 15, 31

    ccr &= ~(CC_Z | CC_N);
    ccr |= (val == 0 ? CC_Z : 0) | (val & highBit ? CC_N : 0);
}

void H8CPU::updateFlag(int flag, bool set)
{
    ccr = (ccr & ~flag) | (set ? flag : 0);
}

int H8CPU::fetchTiming(int len) const
{
    // len is in words (matches the table in the manual)

    // assume PC hasn't crossed memory regions
    // i hope that isn't legal...
    return wordAccessTiming(pc) * len;
}

int H8CPU::byteAccessTiming(uint32_t addr) const
{
    // on-chip rom
    if(addr < romSize)
        return 1;

    int area = (addr /*/ 0x200000*/ >> 21) & 7;

    // area 1-6 is easy (+ the rest of 0)
    if(area < 7)
        return areaAccessCycles8[area];

    // on-chip ram
    if(addr >= 0xFFDC00 && addr < 0xFFFC00)
        return 1;

    if((addr >= 0xFFFE50 && addr < 0xFFFF08) || addr >= 0xFFFF28)
        return 2; // on-chip module

    return areaAccessCycles8[7];
}

int H8CPU::wordAccessTiming(uint32_t addr) const
{
    // on-chip rom
    if(addr < romSize)
        return 1;

    int area = (addr /*/ 0x200000*/ >> 21) & 7;

    // area 1-6 is easy (+ the rest of 0)
    if(area < 7)
        return areaAccessCycles16[area];

    // on-chip ram
    if(addr >= 0xFFDC00 && addr < 0xFFFC00)
        return 1;

    if((addr >= 0xFFFE50 && addr < 0xFFFF08) || addr >= 0xFFFF28)
        return 2; // TODO: on-chip module 8/16 bit

    return areaAccessCycles16[7];
}

void H8CPU::updateBusTimings()
{
    for(int i = 0; i < 8; i++)
    {
        if(busWaitControl & (1 << i)) // waits enabled
            areaAccessCycles8[i] = 3 + ((busWaits >> (i * 2)) & 3);
        else
            areaAccessCycles8[i] = 2;

        if(busWidthControl & (1 << i)) // 8-bit
            areaAccessCycles16[i] = areaAccessCycles8[i] * 2;
        else
            areaAccessCycles16[i] = areaAccessCycles8[i];
    }
}

// assuming little endian here
template<>
uint8_t H8CPU::reg(int r) const
{
    return reinterpret_cast<const uint8_t *>(er + (r & 7))[(r >> 3) ^ 1];
}

template<>
uint8_t &H8CPU::reg(int r)
{
    return reinterpret_cast<uint8_t *>(er + (r & 7))[(r >> 3) ^ 1];
}

template<>
uint16_t H8CPU::reg(int r) const
{
    return reinterpret_cast<const uint16_t *>(er + (r & 7))[r >> 3];
}

template<>
uint16_t &H8CPU::reg(int r)
{
    return reinterpret_cast<uint16_t *>(er + (r & 7))[r >> 3];
}

template<>
uint32_t H8CPU::reg(int r) const
{
    return er[r];
}

template<>
uint32_t &H8CPU::reg(int r)
{
    return er[r];
}

uint8_t H8CPU::readByte(uint32_t addr)
{
    // easily mapped memories
    int area = (addr /*/ 0x200000*/ >> 21) & 7;
    auto ptr = areaPtrs[area];
    if(ptr)
        return ptr[addr & areaMasks[area]];

    // high bits are just ignored
    addr &= 0xFFFFFF;

    // internal
    if(addr < romSize)
        return rom[addr];

    if(addr >= 0xFFDC00 && addr < 0xFFFC00)
        return ram[addr - 0xFFDC00];

    if((addr >= 0xFFFE50 && addr < 0xFFFF08) || (addr >= 0xFFFF28 && addr <= 0xFFFFFF))
        return readIOReg(addr);

    // external devices
    auto &extArea = externalAreas[area];

    if(extArea)
        return extArea->read(addr);// & 0x1FFFFF); // device classes need to mask anyway

    std::cout << "unhandled external read " << std::hex << addr << " @~" << pc << std::dec << std::endl;
    dumpRegs();

    return 0xFF;
}

uint16_t H8CPU::readWord(uint32_t addr)
{
    return (readByte(addr) << 8) | readByte(addr + 1);
}

uint32_t H8CPU::readLong(uint32_t addr)
{
    // happens in realloc(NULL, ...), called from buffer init
    // apparently works fine, so avoid complaining about it...
    if(addr == 0xFFFFFFFC && pc == 0x4a274a)
        return 0;

    // optimise external reads for the areas that don't overlap
    /*if(addr > romSize && addr < 0xFFDC00 && (addr & 0x1FFFFF) <= 0x1FFFFC)
    {
        const auto &extArea = externalAreas[addr >> 21];
        if(extArea)
            return (extArea->read(addr) << 24) | (extArea->read(addr + 1) << 16) | (extArea->read(addr + 2) << 8) | extArea->read(addr + 3);
    }*/

    // slower path
    return (readByte(addr) << 24) | (readByte(addr + 1) << 16) | (readByte(addr + 2) << 8) | readByte(addr + 3);
}

uint32_t H8CPU::readAddr16(uint32_t addr)
{
    //16-bit addresses get sign extended
    uint32_t ret = readWord(addr);
    if(ret & 0x8000)
        ret |= 0xFF0000;

    return ret;
}

uint32_t H8CPU::readDisp16(uint32_t addr)
{
    //16-bit displacements get sign extended
    uint32_t ret = readWord(addr);
    if(ret & 0x8000)
        ret |= 0xFFFF0000;

    return ret;
}

void H8CPU::writeByte(uint32_t addr, uint8_t val)
{
    // high bits are just ignored (tested on hardware)
    addr &= 0xFFFFFF;

    auto &extArea = externalAreas[addr / 0x200000];

    if(addr < romSize)
    {
        std::cerr << "rom write " << std::hex << addr << " = " << static_cast<int>(val) << " @~" << pc << std::dec << std::endl;
    }
    else if(addr >= 0xFFDC00 && addr < 0xFFFC00)
        ram[addr - 0xFFDC00] = val;
    else if((addr >= 0xFFFE50 && addr < 0xFFFF08) || addr >= 0xFFFF28)
        writeIOReg(addr, val);
    else if(extArea)
        extArea->write(addr & 0x1FFFFF, val);
    else
        std::cout << "external write " << std::hex << addr << " = " << static_cast<int>(val) << std::dec << std::endl;
}

void H8CPU::writeWord(uint32_t addr, uint16_t val)
{
    writeByte(addr, val >> 8);
    writeByte(addr + 1, val & 0xFF);
}

void H8CPU::writeLong(uint32_t addr, uint32_t val)
{
    writeByte(addr, val >> 24);
    writeByte(addr + 1, (val >> 16) & 0xFF);
    writeByte(addr + 2, (val >> 8) & 0xFF);
    writeByte(addr + 3, val & 0xFF);
}

uint8_t H8CPU::readIOReg(uint32_t addr)
{
    addr = addr & 0xFFF;

    // TPU 3 - 5
    if(addr >= 0xE80 && addr <= 0xEAB)
    {
        int chan = (addr - 0xE80) / 16 + 3;
        tpuChannels[chan].update(*this);
        return tpuChannels[chan].getReg(addr & 0xF);
    }

    // DMAC
    if(addr >= 0xEE0 && addr <= 0xEFF)
        return dmaChannels[(addr & 0x10) >> 4].getReg(addr & 0xF);


    // PxDDR
    if((addr & 0xFF0) == 0xEB0)
        return ioPorts[addr & 0xF].getDirection();

    // Timer 0/1
    if(addr >= 0xFB0 && addr <= 0xFB9)
        return timers[addr & 1].getReg((addr - 0xFB0) / 2);

    // PORTx
    if((addr & 0xFF0) == 0xF50)
    {
        //std::cout << "port read (reg " << std::hex << addr << ") at ~" << pc << std::dec << "\n";
        return ioPorts[addr & 0xF].read();
    }

    // PxDR
    if((addr & 0xFF0) == 0xF60)
        return ioPorts[addr & 0xF].getWrittenData();

    // TPU 0 - 2
    if(addr >= 0xFD0 && addr <= 0xFFB)
    {
        int chan = (addr - 0xFD0) / 16;
        tpuChannels[chan].update(*this);
        return tpuChannels[chan].getReg(addr & 0xF);
    }

    switch(addr)
    {
        case 0xED7: // DRAMCR
            return 0;

        case 0xF06: // DMABCR
            return (dmaChannels[0].getReg(0x12) & 0x53) | (dmaChannels[1].getReg(0x12) & 0xAC); // make these appear as 0x12-13
        case 0xF07:
            return (dmaChannels[0].getReg(0x13) & 0x33) | (dmaChannels[1].getReg(0x13) & 0xCC); // also merge

        case 0xF2E: // IER
            return irqEnable;

        case 0xF2F: // ISR
            return irqStatus;

        // DTCER
        case 0xF30:
        case 0xF31:
        case 0xF32:
        case 0xF33:
        case 0xF34:
        case 0xF35:
            return dtcEnable[addr & 0xF];

        case 0xF3C: // MSTPCRH
            return mstpcr >> 8;
        case 0xF3D: // MSTPCRL
            return mstpcr & 0xFF;

        case 0xF78: // SMR0
        case 0xF79: // BRR0
        case 0xF7A: // SCR0
        case 0xF7B: // TDR0
        case 0xF7C: // SSR0
        case 0xF7D: // RDR0
        case 0xF7E: // SCMR0
            return serialPorts[0].getReg(addr & 0x7);

        case 0xF80: // SMR1
        case 0xF81: // BRR1
        case 0xF82: // SCR1
        case 0xF83: // TDR1
        case 0xF84: // SSR1
        case 0xF85: // RDR1
        case 0xF86: // SCMR1
            return serialPorts[1].getReg(addr & 0x7);

        case 0xF88: // SMR2
        case 0xF89: // BRR2
        case 0xF8A: // SCR2
        case 0xF8B: // TDR2
        case 0xF8C: // SSR2
        case 0xF8D: // RDR2
        case 0xF8E: // SCMR2
            return serialPorts[2].getReg(addr & 0x7);

        case 0xF90: // ADDRAH
        case 0xF92: // ADDRBH
        case 0xF94: // ADDRCH
        case 0xF96: // ADDRDH
            return a2dData[(addr >> 1) & 3] >> 8;

        case 0xF91: // ADDRAL
        case 0xF93: // ADDRBL
        case 0xF95: // ADDRCL
        case 0xF97: // ADDRDL
            return a2dData[(addr >> 1) & 3] & 0xFF;

        case 0xF98: // ADCSR
            return a2dControlStatus;

        case 0xFC0:
            return tpuStart;
    }

    std::cout << "io reg read " << std::hex << addr << " @~" << pc << std::dec << std::endl;

    return 0;
}

void H8CPU::writeIOReg(uint32_t addr, uint8_t val)
{
    addr = addr & 0xFFF;

    // TPU 3 - 5
    if(addr >= 0xE80 && addr <= 0xEAB)
    {
        int chan = (addr - 0xE80) / 16 + 3;
        int reg = addr & 0xF;
        tpuChannels[chan].update(*this);

        if(reg == 4) // TIER
        {
            if(val)
                tpuInterruptEnable |= 1 << chan;
            else
                tpuInterruptEnable &= ~(1 << chan);
        }

        tpuChannels[chan].setReg(reg, val);
        return;
    }

    // DMAC
    if(addr >= 0xEE0 && addr <= 0xEFF)
    {
        dmaChannels[(addr & 0x10) >> 4].setReg(addr & 0xF, val);
        return;
    }

    // PxDDR
    if((addr & 0xFF0) == 0xEB0)
    {
        ioPorts[addr & 0xF].setDirection(val);
        return;
    }

    // Timer 0/1
    if(addr >= 0xFB0 && addr <= 0xFB9)
    {
        timers[addr & 1].setReg((addr - 0xFB0) / 2, val);
        return;
    }

    // PxDR
    if((addr & 0xFF0) == 0xF60)
    {
        ioPorts[addr & 0xF].write(val);
        return;
    }

    // TPU 0 - 2
    if(addr >= 0xFD0 && addr <= 0xFFB)
    {
        int chan = (addr - 0xFD0) / 16;
        int reg = addr & 0xF;
        tpuChannels[chan].update(*this);

        if(reg == 4) // TIER
        {
            if(val)
                tpuInterruptEnable |= 1 << chan;
            else
                tpuInterruptEnable &= ~(1 << chan);
        }

        tpuChannels[chan].setReg(reg, val);
        return;
    }

    switch(addr)
    {
        case 0xED0: // ABWCR
            busWidthControl = val;
            updateBusTimings();
            break;
        case 0xED1: // ASTCR
            busWaitControl = val;
            updateBusTimings();
            break;
        case 0xED2: // WCRH
            busWaits = (busWaits & 0xFF) | val << 8;
            updateBusTimings();
            break;
        case 0xED3: // WCRL
            busWaits = (busWaits & 0xFF00) | val;
            updateBusTimings();
            break;

        case 0xED7: // DRAMCR
            break;

        //case 0xF00: // DMAWER
        //case 0xF01: // DMATCR
        case 0xF02: // DMACR0A
        case 0xF03: // DMACR0B
            dmaChannels[0].setReg(addr - 0xEF2, val); // make these appear as 0x10-11
            break;
        case 0xF04: // DMACR1A
        case 0xF05: // DMACR1B
            dmaChannels[1].setReg(addr - 0xEF4, val); // make these appear as 0x10-11
            break;
        case 0xF06: // DMABCR
        case 0xF07:
            dmaChannels[0].setReg(addr - 0xEF4, val); // make these appear as 0x12-13
            dmaChannels[1].setReg(addr - 0xEF4, val); // make these appear as 0x12-13
            break;

        // DTCER
        case 0xF30:
        case 0xF31:
        case 0xF32:
        case 0xF33:
        case 0xF34:
        case 0xF35:
            dtcEnable[addr & 0xF] = val;
            break;

        case 0xF2E: // IER
            irqEnable = val;

            // flag newly unmasked IRQs
            if(irqEnable & irqStatus)
            {
                auto servicable = irqEnable & irqStatus;
                for(int i = 0; i < 8; i++)
                {
                    if(servicable & (1 << i))
                        interrupt(static_cast<InterruptSource>(static_cast<int>(InterruptSource::IRQ0) + i));
                }
            }
            break;
        case 0xF2F: // ISR
            irqStatus &= val; // not possible to set bits
            break;

        case 0xF3C: // MSTPCRH
            mstpcr = (val << 8) | (mstpcr & 0xFF);
            break;
        case 0xF3D: // MSTPCRL
            mstpcr = val | (mstpcr & 0xFF00);
            break;

        case 0xF78: // SMR0
        case 0xF79: // BRR0
        case 0xF7A: // SCR0
        case 0xF7B: // TDR0
        case 0xF7C: // SSR0
        case 0xF7D: // RDR0
        case 0xF7E: // SCMR0
            serialPorts[0].setReg(addr & 0x7, val, *this);
            break;

        case 0xF80: // SMR1
        case 0xF81: // BRR1
        case 0xF82: // SCR1
        case 0xF83: // TDR1
        case 0xF84: // SSR1
        case 0xF85: // RDR1
        case 0xF86: // SCMR1
            serialPorts[1].setReg(addr & 0x7, val, *this);
            break;

        case 0xF88: // SMR2
        case 0xF89: // BRR2
        case 0xF8A: // SCR2
        case 0xF8B: // TDR2
        case 0xF8C: // SSR2
        case 0xF8D: // RDR2
        case 0xF8E: // SCMR2
            serialPorts[2].setReg(addr & 0x7, val, *this);
            break;

        case 0xF98: // ADCSR
        {
            a2dControlStatus = val & (0x7F | a2dControlStatus); // not possible to set ADF

            int clock = 2/*CKS1 default*/ | ((a2dControlStatus >> 3) & 1);
            const int convTimes[]{530, 68, 266, 134};
            a2dConversionTime = a2dConvCounter = convTimes[clock];

            int channel = val & 0x7;
            a2dChannel = a2dEndChannel = channel;

            if(val & (1 << 4)) // SCAN
                a2dChannel = channel & 4; // start is either 0 or 4

            break;
        }

        case 0xFBC: //watchdog TCNT/TCSR
        case 0xFBD: //weirdness
            break;

        case 0xFC0: // TSTR
        {
            auto changed = tpuStart ^ val;
            for(int i = 0; i < 6; i++)
            {
                // update any channels that are getting stopped/started
                if(changed & (1 << i))
                    tpuChannels[i].update(*this);
            }
            tpuStart = val;
            break;
        }

        default:
            //std::cout << "io reg write " << std::hex << addr << " = " << static_cast<int>(val) << " @~" << pc << std::dec << std::endl;
            break;
    }
}
