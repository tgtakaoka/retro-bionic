#ifndef __MC68HC12_INIT_H__
#define __MC68HC12_INIT_H__

#include "device.h"

namespace debugger {
namespace mc68hc12 {

struct RegsMc68hc12;

/**
 * The MC68HC912BD32's internal memory map, as reset leaves it in special
 * expanded wide mode: registers, RAM and EEPROM inside, the rest outside.
 * Another chip of the family derives with its own map and setup.
 */
struct Mc68hc12Init : Device {
    Mc68hc12Init() : Device() { enable(true); }

    const char *name() const override { return "INIT"; }
    const char *description() const override;
    void print() const override;

    void reset() override {}
    void loop() override {}
    uint32_t baseAddr() const override { return REG_BASE; }

    // Clears the reset's external access stretch and stops the COP.
    virtual void configSystem(RegsMc68hc12 *regs) const;

    virtual bool is_internal(uint16_t addr) const;
    // SC0BDH, the first SCI0 register.
    virtual uint16_t sciBase() const { return REG_BASE + SC0BDH; }
    // The libasm CPU the disassembler takes.
    virtual const char *cpuType() const { return "68HC12"; }
    // Whether the CCR is 16 bits, stacked as two bytes.
    virtual bool wideCcr() const { return false; }

    static constexpr uint16_t REG_BASE = 0x0000;
    static constexpr uint16_t REG_SIZE = 0x0200;
    static constexpr uint16_t RAM_BASE = 0x0800;
    static constexpr uint16_t RAM_SIZE = 0x0400;
    static constexpr uint16_t EEPROM_BASE = 0x0D00;
    static constexpr uint16_t EEPROM_SIZE = 0x0300;
    // Registers
    static constexpr uint16_t MISC = 0x13;
    static constexpr uint16_t COPCTL = 0x16;
    static constexpr uint16_t SC0BDH = 0xC0;
    static constexpr uint16_t SC0BDL = 0xC1;

protected:
    static void printRange(const char *name, uint16_t base, uint16_t size);
};

}  // namespace mc68hc12
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
