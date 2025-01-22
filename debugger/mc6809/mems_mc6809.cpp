#include "mems_mc6809.h"
#include <asm_mc6809.h>
#include <dis_mc6809.h>
#include "devs_mc6809.h"
#include "regs_mc6809.h"

namespace debugger {
namespace mc6809 {

MemsMc6809::MemsMc6809(Devs *devs)
    : DmaMemory(Endian::ENDIAN_BIG), _devs(devs) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::mc6809::AsmMc6809();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::mc6809::DisMc6809();
#endif
}

uint16_t MemsMc6809::read(uint32_t addr) const {
    return _devs->isSelected(addr) ? _devs->read(addr) : read_byte(addr);
}

void MemsMc6809::write(uint32_t addr, uint16_t data) const {
    if (_devs->isSelected(addr)) {
        _devs->write(addr, data);
    } else {
        write_byte(addr, data);
    }
}

}  // namespace mc6809
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
