#include "mc9s12x_init.h"
#include "debugger.h"
#include "regs_mc68hc12.h"

namespace debugger {
namespace mc68hc12 {

void Mc9s12xInit::print() const {
    printRange("  Registers at ", REG_BASE, X_REG_SIZE);
    printRange("  EEPROM    at ", X_EEPROM_BASE, X_EEPROM_SIZE);
    printRange("  RAM       at ", X_RAM_BASE, X_RAM_SIZE);
}

// The EEPROM window and the fixed EEPROM page are contiguous globally, as
// are the RAM window and the two fixed RAM pages (S12XMMCV2).
uint16_t Mc9s12xInit::local(uint32_t global) {
    if (global - G_RAM_BASE < X_RAM_SIZE)
        return X_RAM_BASE + (global - G_RAM_BASE);
    if (global - G_EEPROM_BASE < X_EEPROM_SIZE)
        return X_EEPROM_BASE + (global - G_EEPROM_BASE);
    return global;  // registers, and outside
}

void Mc9s12xInit::configSystem(RegsMc68hc12 *regs) const {
    // Every reset sets CME and SCME: EXTAL slower than the clock monitor
    // allows, as when the debugger stops it, would move the bus to the
    // self-clock. Clearing them makes a stopped EXTAL stop the chip.
    regs->internal_write(REG_BASE + PLLCTL, 0x00);
    // Special modes let COPCTL be written any time; CR2:0=000 stops the
    // COP whatever the Flash configured.
    regs->internal_write(REG_BASE + COPCTL, 0x00);
}

}  // namespace mc68hc12
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
