#include "mems_i8048.h"
#include <asm_i8048.h>
#include <dis_i8048.h>
#include "regs_i8048.h"

namespace debugger {
namespace i8048 {

ProgI8048::ProgI8048(Mems *data) : DmaMemory(Endian::ENDIAN_BIG), _data(data) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::i8048::AsmI8048();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::i8048::DisI8048();
#endif
}

DataI8048::DataI8048(RegsI8048 *regs, Devs *devs)
    : DmaMemory(Endian::ENDIAN_BIG), _regs(regs), _devs(devs) {}

uint16_t DataI8048::read(uint32_t addr) const {
    addr &= UINT8_MAX;
    return _devs->isSelected(addr) ? _devs->read(addr) : read_byte(addr);
}

void DataI8048::write(uint32_t addr, uint16_t data) const {
    addr &= UINT8_MAX;
    if (_devs->isSelected(addr)) {
        _devs->write(addr, data);
    } else {
        write_byte(addr, data);
    }
}

uint16_t DataI8048::get_data(uint32_t addr) const {
    if (addr < 0x100) {
        return _regs->read_internal(addr);
    } else {
        return read(addr);
    }
}

void DataI8048::put_data(uint32_t addr, uint16_t data) const {
    if (addr < 0x100) {
        _regs->write_internal(addr, data);
    } else {
        write(addr, data);
    }
}

}  // namespace i8048
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
