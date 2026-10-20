#ifndef __MC9S12X_INIT_H__
#define __MC9S12X_INIT_H__

#include "mc68hc12_init.h"

namespace debugger {
namespace mc68hc12 {

/**
 * The MC9S12XDP512's internal memory map, as reset leaves it in special
 * test mode with ROMCTL low: registers, EEPROM and RAM below $4000, the
 * Flash off, the rest outside. The bus shows global addresses; the pages
 * stay as reset sets them.
 */
struct Mc9s12xInit final : Mc68hc12Init {
    void print() const override;

    // Stops the clock monitor first, then the COP.
    void configSystem(RegsMc68hc12 *regs) const override;

    bool is_internal(uint16_t addr) const override { return addr < EXT_BASE; }
    uint16_t sciBase() const override { return REG_BASE + SCI0BDH; }
    const char *cpuType() const override { return "68HCS12X"; }
    bool wideCcr() const override { return true; }

    // The local address of a global one the bus shows, with RPAGE=$FD,
    // EPAGE=$FE and PPAGE=$FE.
    static uint16_t local(uint32_t global);
    // Whether a global address is outside, the debugger's to answer.
    static bool is_external(uint32_t global) {
        return (global >> 16) == EXT_PAGE;
    }

    static constexpr uint16_t X_REG_SIZE = 0x0800;
    static constexpr uint16_t X_EEPROM_BASE = 0x0800;
    static constexpr uint16_t X_EEPROM_SIZE = 0x0800;
    static constexpr uint16_t X_RAM_BASE = 0x1000;
    static constexpr uint16_t X_RAM_SIZE = 0x3000;
    static constexpr uint16_t EXT_BASE = 0x4000;
    // Global addresses
    static constexpr uint32_t G_EEPROM_BASE = 0x13F800;
    static constexpr uint32_t G_RAM_BASE = 0x0FD000;
    static constexpr uint8_t EXT_PAGE = 0x7F;
    // Registers
    static constexpr uint16_t PLLCTL = 0x3A;
    static constexpr uint16_t COPCTL = 0x3C;
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
