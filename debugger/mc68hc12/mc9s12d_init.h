#ifndef __MC9S12D_INIT_H__
#define __MC9S12D_INIT_H__

#include "mc68hc12_init.h"

namespace debugger {
namespace mc68hc12 {

/**
 * The MC9S12DG128B's internal memory map, as reset leaves it in special
 * test mode: registers over 8K RAM from $0000, the EEPROM hidden under
 * them, the rest outside. The Flash is off (ROMON=0).
 */
struct Mc9s12dInit final : Mc68hc12Init {
    void print() const override;

    // Stops the clock monitor first, then drops the external access
    // stretch and shows the free cycles on NOACC.
    void configSystem(RegsMc68hc12 *regs) const override;

    bool is_internal(uint16_t addr) const override;
    uint16_t sciBase() const override { return REG_BASE + SCI0BDH; }
    const char *cpuType() const override { return "68HCS12"; }

    static constexpr uint16_t S12_REG_SIZE = 0x0400;
    static constexpr uint16_t S12_RAM_BASE = 0x0000;
    static constexpr uint16_t S12_RAM_SIZE = 0x2000;
    // Registers
    static constexpr uint16_t PEAR = 0x0A;
    static constexpr uint16_t MISC = 0x13;
    static constexpr uint16_t PLLCTL = 0x3A;
    static constexpr uint16_t SCI0BDH = 0xC8;
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
