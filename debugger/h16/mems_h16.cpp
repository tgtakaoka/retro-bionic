#include "mems_h16.h"
#include <asm_h16.h>
#include <dis_h16.h>

namespace debugger {
namespace h16 {

MemsH16::MemsH16() : ExtMemory(Endian::ENDIAN_BIG) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::h16::AsmH16();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::h16::DisH16();
#endif
}

}  // namespace h16
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
