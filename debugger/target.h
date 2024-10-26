#ifndef __DEBUGGER_TARGET_H__
#define __DEBUGGER_TARGET_H__

#include "devs.h"
#include "mems.h"
#include "pins.h"
#include "regs.h"

namespace debugger {
struct Target {
    static Target &readIdentity();
    static void writeIdentity(const char *identity);
    static void printIdentity();

    Target(const char *id, Pins &pins, Regs &regs, Mems &mems, Devs &devs);

    const char *identity() const { return _identity; }

    void begin() const;
    void reset() const;
    void run() const;
    bool step(bool show) const;
    void idle() const;
    void printCycles() const;

    void assertInt(uint8_t name = 0) const;
    void negateInt(uint8_t name = 0) const;

    bool printBreakPoints() const;
    bool setBreakPoint(uint32_t addr) const;
    bool clearBreakPoint(uint8_t index) const;
    bool isOnBreakPoint() const;

    void printRegisters() const;
    void printStatus() const;
    uint8_t validRegister(const char *word, uint32_t &max) const;
    void setRegister(uint8_t reg, uint32_t value) const;
    void helpRegisters() const;

    uint32_t maxAddr() const;
    uint32_t assemble(uint32_t addr, const char *line) const;
    uint32_t disassemble(uint32_t addr, uint8_t numInsn) const;
    uint16_t read_memory(uint32_t addr, const char *space = nullptr) const;
    uint16_t get_inst(uint32_t addr) const;
    void put_inst(uint32_t addr, uint16_t data) const;
    void write_memory(uint32_t addr, const uint8_t *buffer, uint8_t len) const;
    void write_code(uint32_t addr, const uint8_t *buffer, uint8_t len) const;
    bool hasRomArea() const;
    bool printRomArea() const;
    void setRomArea(uint32_t begin, uint32_t end) const;

    void printDevices() const;
    Device &parseDevice(const char *word) const;
    void enableDevice(Device &dev) const;

private:
    const char *const _identity;
    Pins &_pins;
    Regs &_regs;
    Mems &_mems;
    Devs &_devs;
    Target *const _next;

    static Target *_head;
    static Target &searchIdentity(const char *identity);
};
}  // namespace debugger

#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
