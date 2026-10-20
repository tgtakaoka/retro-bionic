#include "mc9s12d_init.h"
#include "debugger.h"
#include "regs_mc68hc12.h"

namespace debugger {
namespace mc68hc12 {

void Mc9s12dInit::print() const {
    printRange("  Registers at ", REG_BASE, S12_REG_SIZE);
    printRange("  RAM       at ", S12_RAM_BASE, S12_RAM_SIZE);
}

// PORTA/PORTB and DDRA/DDRB ($0000-$0003) leave the map in the expanded
// modes and are read outside; they count as internal all the same, so that
// the debugger never drives the bus against the RAM under them.
bool Mc9s12dInit::is_internal(uint16_t addr) const {
    return static_cast<uint16_t>(addr - S12_RAM_BASE) < S12_RAM_SIZE ||
           static_cast<uint16_t>(addr - REG_BASE) < S12_REG_SIZE;
}

void Mc9s12dInit::configSystem(RegsMc68hc12 *regs) const {
    // Every reset sets CME and SCME: EXTAL slower than 200kHz, as when the
    // debugger stops it, would move the bus to the self-clock. Clearing
    // them makes a stopped EXTAL stop the chip. PLLON goes too, as the PLL
    // is never used (AN2727).
    regs->internal_write(REG_BASE + PLLCTL, 0x00);
    // Special test mode lets MISC and PEAR be written any time.
    // EXSTR1:0=00 drops the three-cycle stretch reset puts on every
    // external access; ROMON stays 0, the Flash off.
    regs->internal_write(REG_BASE + MISC, 0x00);
    // NOACCE shows the free cycles on PE7; PIPOE, LSTRE and RDWE stay set,
    // NECLK clear.
    constexpr uint8_t NOACCE = 0x80;
    constexpr uint8_t PIPOE = 0x20;
    constexpr uint8_t LSTRE = 0x08;
    constexpr uint8_t RDWE = 0x04;
    regs->internal_write(REG_BASE + PEAR, NOACCE | PIPOE | LSTRE | RDWE);
}

}  // namespace mc68hc12
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
