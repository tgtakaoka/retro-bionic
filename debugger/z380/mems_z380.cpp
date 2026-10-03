#include "mems_z380.h"
#include <asm_z380.h>
#include <dis_z380.h>

namespace debugger {
namespace z380 {

MemsZ380::MemsZ380() : ExtMemory(Endian::ENDIAN_LITTLE) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::z380::AsmZ380();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::z380::DisZ380();
#endif
}

void MemsZ380::setMode(bool extended, bool longWord) {
#ifdef WITH_DISASSEMBLER
    _disassembler->setOption("extmode", extended ? "on" : "off");
    _disassembler->setOption("lwordmode", longWord ? "on" : "off");
#endif
}

}  // namespace z380
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
