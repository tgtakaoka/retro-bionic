#include "mems_mc6802.h"
#include <asm_mc6800.h>
#include <dis_mc6800.h>
#include "regs_mc6802.h"

namespace debugger {
namespace mc6802 {

uint16_t MemsMc6802::get_data(uint32_t addr) const {
    return is_internal(addr) ? _regs->internal_read(addr) : read(addr);
}

void MemsMc6802::put_data(uint32_t addr, uint16_t data) const {
    if (is_internal(addr)) {
        _regs->internal_write(addr, data);
    } else {
        write(addr, data);
    }
}

}  // namespace mc6802
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
