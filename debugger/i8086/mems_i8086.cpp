#include "mems_i8086.h"
#include <asm_i8086.h>
#include <dis_i8086.h>

namespace debugger {
namespace i8086 {

MemsI8086::MemsI8086(bool bus8, bool fpu)
    : ExtMemory(Endian::ENDIAN_LITTLE, !bus8), _fpu(fpu) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::i8086::AsmI8086();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::i8086::DisI8086();
#endif
}

#ifdef WITH_ASSEMBLER
libasm::Assembler *MemsI8086::assembler() const {
    auto as = Mems::assembler();
    if (as)
        as->setOption("fpu", _fpu ? "8087" : "off");
    return as;
}
#endif

#ifdef WITH_DISASSEMBLER
libasm::Disassembler *MemsI8086::disassembler() const {
    auto dis = Mems::disassembler();
    if (dis)
        dis->setOption("fpu", _fpu ? "8087" : "off");
    return dis;
}
#endif

}  // namespace i8086
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
