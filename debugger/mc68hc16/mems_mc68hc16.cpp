#include "mems_mc68hc16.h"
#include <asm_mc68hc16.h>
#include <dis_mc68hc16.h>

namespace debugger {
namespace mc68hc16 {

MemsMc68hc16::MemsMc68hc16() : ExtMemory(Endian::ENDIAN_BIG, true) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::mc68hc16::AsmMc68HC16();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::mc68hc16::DisMc68HC16();
#endif
}

}  // namespace mc68hc16
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
