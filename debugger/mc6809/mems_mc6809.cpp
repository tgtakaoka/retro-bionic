#include "mems_mc6809.h"
#include <asm_mc6809.h>
#include <dis_mc6809.h>

namespace debugger {
namespace mc6809 {

MemsMc6809::MemsMc6809(DevsMc6809 *devs)
    : DmaMemory(Endian::ENDIAN_BIG), _devs(devs) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::mc6809::AsmMc6809();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::mc6809::DisMc6809();
#endif
}

}  // namespace mc6809
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
