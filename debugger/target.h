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

    void begin() const;
    void reset() const;
    void run() const;
    bool step(bool show) const;
    void idle() const;
    void printCycles() const;

    void assertInt(uint8_t name = 0) const;
    void negateInt(uint8_t name = 0) const;

    uint32_t nextIp() const { return _regs->nextIp(); }
    void setBreakPoint(uint32_t addr) const { _pins->setBreakInst(addr); }

    void printRegisters() const;
    void printStatus() const;
    uint8_t validRegister(const char *word, uint32_t &max) const;
    void setRegister(uint8_t reg, uint32_t value) const;
    void helpRegisters() const;

    uint32_t maxAddr() const;
    uint32_t assemble(uint32_t addr, const char *line) const;
    uint32_t disassemble(uint32_t addr, uint8_t numInsn) const;
    void dumpMemory(
            uint32_t addr, uint16_t len, const char *spcae = nullptr) const;
    uint16_t read_memory(uint32_t addr, const char *space = nullptr) const;
    uint16_t get_inst(uint32_t addr) const;
    void put_inst(uint32_t addr, uint16_t data) const;
    void write_memory(uint32_t addr, const uint16_t *buffer, uint8_t len) const;
    void write_code(uint32_t addr, const uint8_t *buffer, uint8_t len) const;
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
