#include "mems_mc68hc12.h"
#include <asm_mc68hc12.h>
#include <dis_mc68hc12.h>
#include "mc68hc12_init.h"
#include "regs_mc68hc12.h"

namespace debugger {
namespace mc68hc12 {

MemsMc68hc12::MemsMc68hc12(RegsMc68hc12 *regs, Devs *devs, Mc68hc12Init &init)
    : DmaMemory(Endian::ENDIAN_BIG), _regs(regs), _devs(devs), _init(init) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::mc68hc12::AsmMc68HC12();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::mc68hc12::DisMc68HC12();
#endif
}

uint16_t MemsMc68hc12::read(uint32_t addr) const {
    return _devs->isSelected(addr) ? _devs->read(addr) : read_byte(addr);
}

void MemsMc68hc12::write(uint32_t addr, uint16_t data) const {
    if (_devs->isSelected(addr)) {
        _devs->write(addr, data);
    } else {
        write_byte(addr, data);
    }
}

uint16_t MemsMc68hc12::get_data(uint32_t addr) const {
    if (_init.is_internal(addr))
        return _regs->internal_read(addr);
    return read(addr);
}

void MemsMc68hc12::put_data(uint32_t addr, uint16_t data) const {
    if (_init.is_internal(addr)) {
        _regs->internal_write(addr, data);
    } else {
        write(addr, data);
    }
}

}  // namespace mc68hc12
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
