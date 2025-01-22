#include <ctype.h>

#include <asm_f3850.h>
#include <dis_f3850.h>

#include "mems_f3850.h"
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

uint16_t MemsF3850::get(uint32_t addr, const char *space) const {
    if (space == nullptr)
        return read_byte(addr);
    if (toupper(*space) == 'I' && addr < 0x10)
        return _regs->read_io(addr);
    if (toupper(*space) == 'R' && addr < 0x40)
        return _regs->read_reg(addr);
    return 0;
}

void MemsF3850::put(uint32_t addr, uint16_t data, const char *space) const {
    if (space == nullptr) {
        write_byte(addr, data);
    } else if (toupper(*space) == 'I' && addr < 0x10) {
        _regs->write_io(addr, data);
    } else if (toupper(*space) == 'R' && addr < 0x40) {
        _regs->write_reg(addr, data);
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
