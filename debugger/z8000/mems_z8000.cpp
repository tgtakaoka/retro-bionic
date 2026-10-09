#include "mems_z8000.h"
#include <asm_z8000.h>
#include <dis_z8000.h>

namespace debugger {
namespace z8000 {

MemsZ8000::MemsZ8000() : ExtMemory(Endian::ENDIAN_BIG, true) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::z8000::AsmZ8000();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::z8000::DisZ8000();
#endif
}

}  // namespace z8000
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
