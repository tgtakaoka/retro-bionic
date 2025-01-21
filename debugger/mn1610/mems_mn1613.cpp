#include "mems_mn1613.h"
#include <asm_mn1610.h>
#include <dis_mn1610.h>

namespace debugger {
namespace mn1613 {

MemsMn1613::MemsMn1613(Devs *devs)
    : ExtMemory(Endian::ENDIAN_BIG), _devs(devs), _max_addr(0x3FFFF) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::mn1610::AsmMn1610();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::mn1610::DisMn1610();
#endif
}

void MemsMn1613::isPrint(const uint8_t *data, char buf[2]) const {
    buf[0] = isprint(data[0]) ? data[0] : '.';
    buf[1] = isprint(data[1]) ? data[1] : '.';
}

uint16_t MemsMn1613::read(uint32_t addr) const {
    return read_word(addr);
}

void MemsMn1613::write(uint32_t addr, uint16_t data) const {
    write_word(addr, data);
}

uint16_t MemsMn1613::get(uint32_t addr, const char *space) const {
    return space == nullptr ? read_word(addr) : _devs->read(addr);
}

void MemsMn1613::put(uint32_t addr, uint16_t data, const char *space) const {
    if (space == nullptr) {
        write_word(addr, data);
    } else {
        _devs->write(addr, data);
    }
}

uint16_t MemsMn1613::get_inst(uint32_t addr) const {
    return read_word(addr);
}

void MemsMn1613::put_inst(uint32_t addr, uint16_t data) const {
    write_word(addr, data);
}

}  // namespace mn1613
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
