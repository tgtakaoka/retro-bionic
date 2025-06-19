#include "mems_i8096.h"
#include <asm_i8096.h>
#include <dis_i8096.h>
#include "regs_i8096.h"

namespace debugger {
namespace i8096 {

MemsI8096::MemsI8096(Devs *devs, RegsI8096 *regs)
    : DmaMemory(Endian::ENDIAN_LITTLE), _devs(devs), _regs(regs) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::i8096::AsmI8096();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::i8096::DisI8096();
#endif
}

uint16_t MemsI8096::read(uint32_t addr) const {
    if (addr == 0x2018) {
        // Chip Configuration Register
        // 8 Bit width, #WR/#BHE, #ADV, infinite READY, no ROM protection
        return 0xF5;
    }
    return _devs->isSelected(addr) ? _devs->read(addr) : read_byte(addr);
}

void MemsI8096::write(uint32_t addr, uint16_t data) const {
    if (_devs->isSelected(addr)) {
        _devs->write(addr, data);
    } else {
        write_byte(addr, data);
    }
}

uint16_t MemsI8096::get_prog(uint32_t addr) const {
    return addr < 0x100 ? _regs->read_data(addr) : read(addr);
}

void MemsI8096::put_prog(uint32_t addr, uint16_t data) const {
    if (addr < 0x100) {
        _regs->write_data(addr, data);
    } else {
        write(addr, data);
    }
}

}  // namespace i8096
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
