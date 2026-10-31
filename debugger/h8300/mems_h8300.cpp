#include "mems_h8300.h"
#include <asm_h8300.h>
#include <dis_h8300.h>

namespace debugger {
namespace h8300 {

MemsH8300::MemsH8300() : DmaMemory(Endian::ENDIAN_BIG) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::h8300::AsmH8300();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::h8300::DisH8300();
#endif
}

}  // namespace h8300
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
