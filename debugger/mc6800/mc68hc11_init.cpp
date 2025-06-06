#include "mc68hc11_init.h"
#include "debugger.h"
#include "regs_mc68hc11.h"

namespace debugger {
namespace mc68hc11 {

const char *Mc68hc11Init::description() const {
    return Debugger.target().cpuName();
}

void Mc68hc11Init::print() const {
    cli.print("  RAM    at ");
    cli.printHex(_ram_base, 4);
    cli.print('-');
    cli.printlnHex(_ram_base + _ram_size - 1, 4);
    cli.print("  Device at ");
    cli.printHex(_dev_base, 4);
    cli.print('-');
    cli.printlnHex(_dev_base + _dev_size, 4);
}

bool Mc68hc11Init::is_internal(uint16_t addr) const {
    if (addr >= _ram_base && addr < _ram_base + _ram_size)
        return true;
    if (addr >= _dev_base && addr < _dev_base + _dev_size)
        return true;
    return false;
}

void Mc68hc11Init::configSystem(RegsMc68hc11 *regs) {
    const auto current = init();
    set(_init);
    // Disable watchdog timer.
    constexpr uint8_t NOCOP = 0x04;
    // constexpr uint8_t ROMON = 0x02;
    regs->internal_write(_dev_base + CONFIG, NOCOP);

    // constexpr uint8_t CME = 0x08;
    regs->internal_write(_dev_base + OPTION, 0);

    // It can be written to only once within the first 64 E-clock
    // cycles after a reset, and then it becomes a read-only register.
    regs->internal_write(_dev_base + INIT, current);
    set(current);
}

}  // namespace mc68hc11
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
