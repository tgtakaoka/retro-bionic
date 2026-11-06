#include "mems_h16.h"
#include "regs_h16.h"
#include <asm_h16.h>
#include <dis_h16.h>

namespace debugger {
namespace h16 {

MemsH16::MemsH16() : ExtMemory(Endian::ENDIAN_BIG) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::h16::AsmH16();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::h16::DisH16();
    // R15 as the register dump names it, SSP or USP
    _disassembler->setOption("sp-alias", "on");
#endif
}

uint16_t MemsH16::get_data(uint32_t addr) const {
    if (_regs && _regs->isInternal(addr))
        return _regs->internalRead(addr);
    return get_prog(addr);
}

void MemsH16::put_data(uint32_t addr, uint16_t data) const {
    if (_regs && _regs->isInternal(addr)) {
        _regs->internalWrite(addr, data);
    } else {
        put_prog(addr, data);
    }
}

}  // namespace h16
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
