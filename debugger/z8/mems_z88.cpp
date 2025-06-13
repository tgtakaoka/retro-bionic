#include "mems_z88.h"
#include "regs_z88.h"

namespace debugger {
namespace z88 {

MemsZ88::MemsZ88(RegsZ88 *regs, Devs *devs) : MemsZ8(devs), _regs(regs) {}

uint16_t MemsZ88::get_data(uint32_t addr) const {
    if (addr < 0x100) {
        return _regs->read_reg(addr, SET_BANK0);
    } else if (addr < 0x200) {
        return _regs->read_reg(addr, SET_BANK1);
    } else if (addr < 0x300) {
        return _regs->read_reg(addr, SET_TWO);
    } else {
        return read(addr);
    }
}

void MemsZ88::put_data(uint32_t addr, uint16_t data) const {
    if (addr < 0x100) {
        _regs->write_reg(addr, data, SET_BANK0);
    } else if (addr < 0x200) {
        _regs->write_reg(addr, data, SET_BANK1);
    } else if (addr < 0x300) {
        _regs->write_reg(addr, data, SET_TWO);
    } else {
        read(addr);
    }
}

}  // namespace z88
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
