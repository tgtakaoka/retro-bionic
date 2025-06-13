#include "mems_f3850.h"
#include <asm_f3850.h>
#include <dis_f3850.h>
#include "regs_f3850.h"

namespace debugger {
namespace f3850 {

MemsF3850::MemsF3850(RegsF3850 *regs)
    : DmaMemory(Endian::ENDIAN_BIG), _regs(regs) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::f3850::AsmF3850();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::f3850::DisF3850();
#endif
}

uint16_t MemsF3850::get_data(uint32_t addr) const {
    return addr < 0x40 ? _regs->read_reg(addr) : read(addr);
}

void MemsF3850::put_data(uint32_t addr, uint16_t data) const {
    if (addr < 0x40) {
        _regs->write_reg(addr, data);
    } else {
        write(addr, data);
    }
}

}  // namespace f3850
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
