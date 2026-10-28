#include "mems_mc68000.h"
#include <asm_mc68000.h>
#include <dis_mc68000.h>

namespace debugger {
namespace mc68000 {

MemsMc68000::MemsMc68000(uint32_t addrMask)
    : ExtMemory(Endian::ENDIAN_BIG, true), _addrMask(addrMask) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::mc68000::AsmMc68000();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::mc68000::DisMc68000();
#endif
}

}  // namespace mc68000
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
