#include "mems_z80.h"
#include <asm_z80.h>
#include <dis_z80.h>

namespace debugger {
namespace z80 {

MemsZ80::MemsZ80() : DmaMemory(Endian::ENDIAN_LITTLE) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::z80::AsmZ80();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::z80::DisZ80();
#endif
}

}  // namespace z80
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
