#include "mems_z86.h"
#include "regs_z86.h"

namespace debugger {
namespace z86 {

MemsZ86::MemsZ86(RegsZ86 *regs, Devs *devs) : MemsZ8(devs), _regs(regs) {};

uint16_t MemsZ86::get_data(uint32_t addr) const {
    return (addr < 0x100) ? _regs->read_reg(addr) : read(addr);
}

void MemsZ86::put_data(uint32_t addr, uint16_t data) const {
    if (addr < 0x100) {
        _regs->write_reg(addr, data);
    } else {
        write(addr, data);
    }
}

}  // namespace z86
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
