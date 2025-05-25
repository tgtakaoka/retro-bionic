#include "mems_tms320.h"
#include <asm_tms320.h>
#include <dis_tms320.h>

namespace debugger {
namespace tms320 {

MemsTms320::MemsTms320() : DmaMemory(Endian::ENDIAN_BIG) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::tms320::AsmTms320();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::tms320::DisTms320();
#endif
}

uint16_t MemsTms320::read(uint32_t addr) const {
    return read_word(addr);
}

void MemsTms320::write(uint32_t addr, uint16_t data) const {
    write_word(addr, data);
}

}  // namespace tms320
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
