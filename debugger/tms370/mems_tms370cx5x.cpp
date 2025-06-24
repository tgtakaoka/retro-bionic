#include "mems_tms370cx5x.h"
#include "pins_tms370cx5x.h"

namespace debugger {
namespace tms370cx5x {

MemsTms370Cx5x::MemsTms370Cx5x(
        tms370::RegsTms370 *regs, PinsTms370Cx5x *pins, Devs *devs)
    : MemsTms370(regs, pins, devs) {}

bool MemsTms370Cx5x::internal(uint32_t addr) const {
    return (addr < 0x0100)                        // internal RAM
           || (addr >= 0x1000 && addr < 0x10C0)   // internal Peripheral
           || (addr >= 0x1F00 && addr < 0x2000);  // internal EEPROM
}

}  // namespace tms370cx5x
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
