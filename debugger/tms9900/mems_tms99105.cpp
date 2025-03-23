#include "mems_tms99105.h"
#include <asm_tms9900.h>
#include <dis_tms9900.h>

namespace debugger {
namespace tms99105 {

MemsTms99105::MemsTms99105(Devs *devs) : MemsTms9900(devs) {}

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
