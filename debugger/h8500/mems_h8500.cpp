#include "mems_h8500.h"
#include <asm_h8500.h>
#include <dis_h8500.h>

namespace debugger {
namespace h8500 {

// Both boards run in maximum mode: 24-bit addresses, CP:PC.
MemsH8500::MemsH8500(uint32_t addrMask)
    : ExtMemory(Endian::ENDIAN_BIG), _addrMask(addrMask) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::h8500::AsmH8500();
    _assembler->setOption("maximum-mode", "on");
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::h8500::DisH8500();
    _disassembler->setOption("maximum-mode", "on");
#endif
}

}  // namespace h8500
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
