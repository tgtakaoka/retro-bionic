#ifndef __DEBUGGER_TARGET_H__
#define __DEBUGGER_TARGET_H__

#include "devs.h"
#include "identity.h"
#include "mems.h"
#include "pins.h"
#include "regs.h"

namespace debugger {

struct Target {
    Target(const Identity *id, Pins *pins);
    ~Target() { delete _pins; }

    const char *identity() const { return _id->name(); }
    const char *cpu() const { return _regs->cpu(); }
    const char *cpuName() const { return _regs->cpuName(); }
    uint8_t addressWidth() const { return _mems->addressWidth(); }
    uint8_t addressUnit() const { return _mems->addressUnit(); }
    uint8_t opCodeWidth() const { return _mems->opCodeWidth(); }
    uint8_t inputRadix() const { return _mems->listRadix(); }
    Endian endian() const { return _mems->endian(); }

    void begin() const;
    void reset() const;
    void run() const;
    bool step(bool show) const;
    void idle() const;
    void printCycles() const;

    void assertInt(uint8_t name = 0) const;
    void negateInt(uint8_t name = 0) const;

    uint32_t nextIp() const { return _regs->nextIp(); }

    /* Read and write instruction code for break point */
    void setBreakPoint(uint32_t addr) const { _pins->setBreakInst(addr); }
    uint16_t getInst(uint32_t addr) const;
    void putInst(uint32_t addr, uint16_t data) const;

    void printRegisters(bool dis = true) const;
    uint_fast8_t validRegister(const char *word, uint32_t &max) const;
    bool setRegister(uint_fast8_t reg, uint32_t value) const;
    void helpRegisters() const;

    uint32_t maxAddr() const;
    uint32_t maxData() const;
    uint32_t assemble(uint32_t addr, const char *line) const;
    uint32_t disassemble(uint32_t addr, uint_fast8_t numInsn) const;
    void dumpMemory(uint32_t addr, uint16_t len, bool prog = false) const;
    void writeMemory(uint32_t addr, const uint16_t *buffer, uint_fast8_t len,
            bool prog = false) const;
    void loadCode(
            uint32_t byte_addr, const uint8_t *code, uint_fast8_t len) const;

    /* Set and prrint read only area */
    bool printRomArea() const;
    void setRomArea(uint32_t begin, uint32_t end) const;

    void printDevices() const;
    Device *parseDevice(const char *word) const;
    void enableDevice(Device *dev) const;

private:
    const Identity *_id;
    Pins *_pins;
    Regs *_regs;
    Mems *_mems;
    Devs *_devs;
};
}  // namespace debugger

#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
