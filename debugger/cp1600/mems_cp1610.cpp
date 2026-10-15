#include "mems_cp1610.h"
#include <asm_cp1600.h>
#include <dis_cp1600.h>

namespace debugger {
namespace cp1600 {

MemsCp1610::MemsCp1610(Devs *devs)
    : DmaMemory(Endian::ENDIAN_BIG, true), _devs(devs) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::cp1600::AsmCp1600();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::cp1600::DisCp1600();
#endif
}

uint16_t MemsCp1610::read(uint32_t addr) const {
    return _devs->isSelected(addr) ? _devs->read(addr) : read_word(addr);
}

void MemsCp1610::write(uint32_t addr, uint16_t data) const {
    if (_devs->isSelected(addr)) {
        _devs->write(addr, data);
    } else {
        write_word(addr, data);
    }
}

void MemsCp1610::isPrint(const uint8_t *data, char buf[2]) const {
    buf[0] = isprint(data[0]) ? data[0] : '.';
    buf[1] = isprint(data[1]) ? data[1] : '.';
}

}  // namespace cp1600
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
