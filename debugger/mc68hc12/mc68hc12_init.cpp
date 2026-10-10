#include "mc68hc12_init.h"
#include "debugger.h"
#include "regs_mc68hc12.h"

namespace debugger {
namespace mc68hc12 {

const char *Mc68hc12Init::description() const {
    return Debugger.target().cpuName();
}

namespace {
void printRange(const char *name, uint16_t base, uint16_t size) {
    cli.print(name);
    cli.printHex(base, 4);
    cli.print('-');
    cli.printlnHex(base + size - 1, 4);
}
}  // namespace

void Mc68hc12Init::print() const {
    printRange("  Registers at ", REG_BASE, REG_SIZE);
    printRange("  RAM       at ", RAM_BASE, RAM_SIZE);
    printRange("  EEPROM    at ", EEPROM_BASE, EEPROM_SIZE);
}

bool Mc68hc12Init::is_internal(uint16_t addr) const {
    const auto in = [addr](uint16_t base, uint16_t size) {
        return static_cast<uint16_t>(addr - base) < size;
    };
    return in(REG_BASE, REG_SIZE) || in(RAM_BASE, RAM_SIZE) ||
           in(EEPROM_BASE, EEPROM_SIZE);
}

void Mc68hc12Init::configSystem(RegsMc68hc12 *regs) const {
    // Special modes let MISC and COPCTL be written any time. EXSTR1:0=00
    // drops the three-cycle stretch reset puts on every external access;
    // ROMON stays 0, the Flash off.
    regs->internal_write(REG_BASE + MISC, 0x00);
    // CR2:0=000 stops the COP; DISR stays set, as special modes leave it.
    constexpr uint8_t DISR = 0x08;
    regs->internal_write(REG_BASE + COPCTL, DISR);
}

}  // namespace mc68hc12
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
