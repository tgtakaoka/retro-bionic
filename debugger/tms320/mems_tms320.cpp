#include "mems_tms320.h"
#include <asm_tms320.h>
#include <dis_tms320.h>

namespace debugger {
namespace tms320 {

MemsTms320::MemsTms320() : DmaMemory(Endian::ENDIAN_LITTLE) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::tms320::AsmTms320();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::tms320::DisTms320();
#endif
}

}  // namespace tms320
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
