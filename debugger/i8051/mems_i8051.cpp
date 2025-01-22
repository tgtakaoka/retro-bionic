#include "mems_i8051.h"
#include <asm_i8051.h>
#include <ctype.h>
#include <dis_i8051.h>
#include "regs_i8051.h"

namespace debugger {
namespace i8051 {

ProgI8051::ProgI8051(RegsI8051 *regs, Mems *data)
    : DmaMemory(Endian::ENDIAN_BIG), _regs(regs), _data(data) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::i8051::AsmI8051();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::i8051::DisI8051();
#endif
}

DataI8051::DataI8051(Devs *devs) : DmaMemory(Endian::ENDIAN_BIG), _devs(devs) {}

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

uint16_t ProgI8051::get(uint32_t addr, const char *space) const {
    if (space == nullptr || toupper(*space) == 'P')
        return read_byte(addr);
    if (toupper(*space) == 'M')
        return _regs->read_internal(addr);
    if (toupper(*space) == 'X')
        return _data->read(addr);
    return 0;
}

void ProgI8051::put(uint32_t addr, uint16_t data, const char *space) const {
    if (space == nullptr || toupper(*space) == 'P') {
        write_byte(addr, data);
    } else if (toupper(*space) == 'M') {
        _regs->write_internal(addr, data);
    } else if (toupper(*space) == 'X') {
        _data->write(addr, data);
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
