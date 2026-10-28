#include "mems_pdp11.h"
#include <asm_pdp11.h>
#include <dis_pdp11.h>

namespace debugger {
namespace pdp11 {

MemsPdp11::MemsPdp11() : DmaMemory(Endian::ENDIAN_LITTLE, true) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::pdp11::AsmPdp11();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::pdp11::DisPdp11();
#endif
}

}  // namespace pdp11
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
