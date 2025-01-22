#include "mems_mc6800.h"
#include <asm_mc6800.h>
#include <dis_mc6800.h>
#include "devs_mc6800.h"
#include "regs_mc6800.h"

namespace debugger {
namespace mc6800 {

MemsMc6800::MemsMc6800(RegsMc6800 *regs, Devs *devs)
    : DmaMemory(Endian::ENDIAN_BIG), _regs(regs), _devs(devs) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::mc6800::AsmMc6800();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::mc6800::DisMc6800();
#endif
}

uint16_t MemsMc6800::read(uint32_t addr) const {
    return _devs->isSelected(addr) ? _devs->read(addr) : read_byte(addr);
}

void MemsMc6800::write(uint32_t addr, uint16_t data) const {
    if (_devs->isSelected(addr)) {
        _devs->write(addr, data);
    } else {
        write_byte(addr, data);
    }
}

}  // namespace mc6800
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
