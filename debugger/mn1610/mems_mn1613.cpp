#include "mems_mn1613.h"
#include <asm_mn1610.h>
#include <dis_mn1610.h>

namespace debugger {
namespace mn1613 {

MemsMn1613::MemsMn1613() : ExtMemory(Endian::ENDIAN_BIG), _max_addr(0x3FFFF) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::mn1610::AsmMn1610();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::mn1610::DisMn1610();
#endif
}

void MemsMn1613::isPrint(const uint8_t *data, char buf[2]) const {
    buf[0] = isprint(data[0]) ? data[0] : '.';
    buf[1] = isprint(data[1]) ? data[1] : '.';
}

}  // namespace mn1613
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
