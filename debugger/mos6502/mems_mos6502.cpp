#include "mems_mos6502.h"
#include <asm_mos6502.h>
#include <dis_mos6502.h>

namespace debugger {
namespace mos6502 {

MemsMos6502::MemsMos6502(DevsMos6502 *devs)
    : ExtMemory(Endian::ENDIAN_LITTLE),
      _devs(devs),
      _longA(false),
      _longI(false) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::mos6502::AsmMos6502();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::mos6502::DisMos6502();
#endif
}

#ifdef WITH_ASSEMBLER
libasm::Assembler *MemsMos6502::assembler() const {
    auto as = Mems::assembler();
    if (as) {
        as->setOption("longa", _longA ? "on" : "off");
        as->setOption("longi", _longI ? "on" : "off");
    }
    return as;
}
#endif

#ifdef WITH_DISASSEMBLER
libasm::Disassembler *MemsMos6502::disassembler() const {
    auto dis = Mems::disassembler();
    if (dis) {
        dis->setOption("longa", _longA ? "on" : "off");
        dis->setOption("longi", _longI ? "on" : "off");
    }
    return dis;
}
#endif

}  // namespace mos6502
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
