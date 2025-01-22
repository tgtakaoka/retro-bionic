#include "mems_tms99105.h"
#include <asm_tms9900.h>
#include <dis_tms9900.h>

namespace debugger {
namespace tms99105 {

MemsTms99105::MemsTms99105(Devs *devs) : MemsTms9900(16, devs) {}

uint16_t MemsTms99105::read(uint32_t addr) const {
    return _devs->isSelected(addr) ? (_devs->read(addr) << 8)
                                   : read16(addr);
}

void MemsTms99105::write(uint32_t addr, uint16_t data) const {
    if (_devs->isSelected(addr)) {
        _devs->write(addr, data >> 8);
    } else {
        write16(addr, data);
    }
}

}  // namespace tms99105
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
