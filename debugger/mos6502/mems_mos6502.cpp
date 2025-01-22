#include "mems_mos6502.h"
#include <asm_mos6502.h>
#include <dis_mos6502.h>

namespace debugger {
namespace mos6502 {

MemsMos6502::MemsMos6502(Devs *devs)
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

uint16_t MemsMos6502::read(uint32_t addr) const {
    return _devs->isSelected(addr) ? _devs->read(addr) : read_byte(addr);
}

void MemsMos6502::write(uint32_t addr, uint16_t data) const {
    if (_devs->isSelected(addr)) {
        _devs->write(addr, data);
    } else {
        write_byte(addr, data);
    }
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
