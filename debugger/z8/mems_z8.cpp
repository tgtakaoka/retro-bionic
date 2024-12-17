#include "mems_z8.h"
#include <asm_z8.h>
#include <dis_z8.h>
#include "regs_z8.h"

namespace debugger {
namespace z8 {

MemsZ8::MemsZ8(RegsZ8 *regs, Devs *devs)
    : DmaMemory(Endian::ENDIAN_BIG), _regs(regs), _devs(devs) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::z8::AsmZ8();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::z8::DisZ8();

#endif
}

uint16_t MemsZ8::read(uint32_t addr) const {
    return _devs->isSelected(addr) ? _devs->read(addr) : raw_read(addr);
}

void MemsZ8::write(uint32_t addr, uint16_t data) const {
    if (_devs->isSelected(addr)) {
        _devs->write(addr, data);
    } else {
        raw_write(addr, data);
    }
}

uint16_t MemsZ8::get(uint32_t addr, const char *space) const {
    if (space == nullptr)
        return read(addr);
    return _regs->read_reg(addr, _regs->parseSpace(space));
}

void MemsZ8::put(uint32_t addr, uint16_t data, const char *space) const {
    if (space == nullptr) {
        write(addr, data);
    } else {
        _regs->write_reg(addr, data, _regs->parseSpace(space));
    }
}

}  // namespace z8
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
