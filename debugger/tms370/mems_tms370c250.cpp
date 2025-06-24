#include "mems_tms370c250.h"
#include "pins_tms370c250.h"

namespace debugger {
namespace tms370c250 {

MemsTms370C250::MemsTms370C250(
        tms370::RegsTms370 *regs, PinsTms370C250 *pins, Devs *devs)
    : MemsTms370(regs, pins, devs) {}

bool MemsTms370C250::internal(uint32_t addr) const {
    return (addr < 0x0100)                        // internal RAM
           || (addr >= 0x1000 && addr < 0x10C0)   // internal Peripheral
           || (addr >= 0x1F00 && addr < 0x2000);  // internal EEPROM
}

}  // namespace tms370c250
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
