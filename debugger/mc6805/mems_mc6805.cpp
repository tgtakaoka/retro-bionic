#include "mems_mc6805.h"
#include <asm_mc6805.h>
#include <dis_mc6805.h>
#include "regs_mc6805.h"

namespace debugger {
namespace mc6805 {

MemsMc6805::MemsMc6805(RegsMc6805 *regs, Devs *devs, uint8_t pc_bits)
    : DmaMemory(Endian::ENDIAN_BIG),
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

uint16_t MemsMc6805::get(uint32_t addr, const char *) const {
    if (is_internal(addr)) {
        return _regs->internal_read(addr);
    } else {
        return read(addr);
    }
}

void MemsMc6805::put(uint32_t addr, uint16_t data, const char *) const {
    if (is_internal(addr)) {
        _regs->internal_write(addr, data);
    } else {
        write(addr, data);
    }
}

}  // namespace mc6805
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
