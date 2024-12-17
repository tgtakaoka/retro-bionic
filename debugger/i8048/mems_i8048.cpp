#include "mems_i8048.h"
#include <asm_i8048.h>
#include <ctype.h>
#include <dis_i8048.h>
#include "regs_i8048.h"

namespace debugger {
namespace i8048 {

ProgI8048::ProgI8048(RegsI8048 *regs, Mems *data)
    : DmaMemory(Endian::ENDIAN_BIG), _regs(regs), _data(data) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::i8048::AsmI8048();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::i8048::DisI8048();
#endif
}

DataI8048::DataI8048(Devs *devs) : DmaMemory(Endian::ENDIAN_BIG), _devs(devs) {}

uint16_t DataI8048::read(uint32_t addr) const {
    addr &= 0xFF;
    return _devs->isSelected(addr) ? _devs->read(addr) : raw_read(addr);
}

void DataI8048::write(uint32_t addr, uint16_t data) const {
    addr &= 0xFF;
    if (_devs->isSelected(addr)) {
        _devs->write(addr, data);
    } else {
        raw_write(addr, data);
    }
}

uint16_t ProgI8048::get(uint32_t addr, const char *space) const {
    if (space == nullptr || toupper(*space) == 'P')
        return raw_read(addr);
    if (toupper(*space) == 'M')
        return _regs->read_internal(addr);
    if (toupper(*space) == 'X')
        return _data->read(addr);
    return 0;
}

void ProgI8048::put(uint32_t addr, uint16_t data, const char *space) const {
    if (space == nullptr || toupper(*space) == 'P') {
        raw_write(addr, data);
    } else if (toupper(*space) == 'M') {
        _regs->write_internal(addr, data);
    } else if (toupper(*space) == 'X') {
        _data->write(addr, data);
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
