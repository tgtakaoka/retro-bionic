#include "mems_mc6805.h"
#include <asm_mc6805.h>
#include <dis_mc6805.h>
#include <stdlib.h>
#include "pins_mc6805.h"
#include "regs_mc6805.h"

namespace debugger {
namespace mc6805 {

MemsMc6805::MemsMc6805(
        PinsMc6805 *pins, RegsMc6805 *regs, Devs *devs, uint8_t pc_bits)
    : DmaMemory(Endian::ENDIAN_BIG),
      _pins(pins),
      _regs(regs),
      _devs(devs),
      _pc_bits(pc_bits),
      _max_addr((1U << pc_bits) - 1) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::mc6805::AsmMc6805();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::mc6805::DisMc6805();
#endif
}

uint16_t MemsMc6805::read(uint32_t addr) const {
    return _devs->isSelected(addr) ? _devs->read(addr) : raw_read(addr);
}

void MemsMc6805::write(uint32_t addr, uint16_t data) const {
    if (_devs->isSelected(addr)) {
        _devs->write(addr, data);
    } else {
        raw_write(addr, data);
    }
}

uint16_t MemsMc6805::get(uint32_t addr, const char *) const {
    if (_pins->is_internal(addr)) {
        return _regs->internal_read(addr);
    } else {
        return read(addr);
    }
}

void MemsMc6805::put(uint32_t addr, uint16_t data, const char *) const {
    if (_pins->is_internal(addr)) {
        _regs->internal_write(addr, data);
    } else {
        write(addr, data);
    }
}

#ifdef WITH_ASSEMBLER
libasm::Assembler *MemsMc6805::assembler() const {
    auto as = Mems::assembler();
    char buf[8];
    as->setOption("pc-bits", itoa(_pc_bits, buf, 10));
    return as;
}
#endif

#ifdef WITH_DISASSEMBLER
libasm::Disassembler *MemsMc6805::disassembler() const {
    auto dis = Mems::disassembler();
    char buf[8];
    dis->setOption("pc-bits", itoa(_pc_bits, buf, 10));
    return dis;
}
#endif

}  // namespace mc6805
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
