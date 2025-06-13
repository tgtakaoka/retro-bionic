#include "mems_i8051.h"
#include <asm_i8051.h>
#include <dis_i8051.h>
#include "regs_i8051.h"

namespace debugger {
namespace i8051 {

ProgI8051::ProgI8051(Mems *data) : DmaMemory(Endian::ENDIAN_BIG), _data(data) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::i8051::AsmI8051();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::i8051::DisI8051();
#endif
}

DataI8051::DataI8051(RegsI8051 *regs, Devs *devs)
    : DmaMemory(Endian::ENDIAN_BIG), _regs(regs), _devs(devs) {}

uint16_t DataI8051::read(uint32_t addr) const {
    return _devs->isSelected(addr) ? _devs->read(addr) : read_byte(addr);
}

void DataI8051::write(uint32_t addr, uint16_t data) const {
    if (_devs->isSelected(addr)) {
        _devs->write(addr, data);
    } else {
        write_byte(addr, data);
    }
}

uint16_t DataI8051::get_data(uint32_t addr) const {
    if (addr < 0x100) {
        return _regs->read_internal(addr);
    } else {
        return read(addr & UINT16_MAX);
    }
}

void DataI8051::put_data(uint32_t addr, uint16_t data) const {
    if (addr < 0x100) {
        _regs->write_internal(addr, data);
    } else {
        write(addr, data & UINT16_MAX);
    }
}

}  // namespace i8051
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
