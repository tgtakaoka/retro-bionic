#include "mems_z280.h"
#include <asm_z280.h>
#include <dis_z280.h>

namespace debugger {
namespace z280 {

MemsZ280::MemsZ280() : DmaMemory(Endian::ENDIAN_LITTLE) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::z280::AsmZ280();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::z280::DisZ280();
#endif
}

}  // namespace z280
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
