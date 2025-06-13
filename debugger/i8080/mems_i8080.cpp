#include "mems_i8080.h"
#include <asm_i8080.h>
#include <dis_i8080.h>

namespace debugger {
namespace i8080 {

MemsI8080::MemsI8080() : DmaMemory(Endian::ENDIAN_LITTLE) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::i8080::AsmI8080();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::i8080::DisI8080();
#endif
}

}  // namespace i8080
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
