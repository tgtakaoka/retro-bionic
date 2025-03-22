#include "mems_tms99105.h"
#include <asm_tms9900.h>
#include <dis_tms9900.h>

namespace debugger {
namespace tms99105 {

MemsTms99105::MemsTms99105(Devs *devs) : MemsTms9900(16, devs) {}

uint16_t MemsTms99105::read(uint32_t byte_addr) const {
    return _devs->isSelected(byte_addr) ? (_devs->read(byte_addr) << 8)
                                        : read_word(byte_addr / 2);
}

void MemsTms99105::write(uint32_t byte_addr, uint16_t data) const {
    if (_devs->isSelected(byte_addr)) {
        _devs->write(byte_addr, data >> 8);
    } else {
        write_word(byte_addr / 2, data);
    }
}

uint16_t MemsTms99105::get_inst(uint32_t byte_addr) const {
    return read_word(byte_addr / 2);
}

void MemsTms99105::put_inst(uint32_t byte_addr, uint16_t data) const {
    write(byte_addr / 2, data);
}

uint16_t MemsTms99105::read_macro(uint32_t byte_addr) const {
    return read_word((byte_addr + MACRO_OFFSET) / 2);
}

void MemsTms99105::write_macro(uint32_t byte_addr, uint16_t data) const {
    write_word((byte_addr + MACRO_OFFSET) / 2, data);
}

}  // namespace tms99105
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
