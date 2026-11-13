#include "mems_mc6800.h"
#include <asm_mc6800.h>
#include <dis_mc6800.h>

namespace debugger {
namespace mc6800 {

MemsMc6800Base::MemsMc6800Base(RegsMc6800 *regs)
    : DmaMemory(Endian::ENDIAN_BIG), _regs(regs) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::mc6800::AsmMc6800();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::mc6800::DisMc6800();
#endif
}

}  // namespace mc6800
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
