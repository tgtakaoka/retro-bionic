#include "mems_z8.h"
#include <asm_z8.h>
#include <dis_z8.h>

namespace debugger {
namespace z8 {

MemsZ8::MemsZ8(Devs *devs) : DmaMemory(Endian::ENDIAN_BIG), _devs(devs) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::z8::AsmZ8();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::z8::DisZ8();

#endif
}

uint16_t MemsZ8::read(uint32_t addr) const {
    return _devs->isSelected(addr) ? _devs->read(addr) : read_byte(addr);
}

void MemsZ8::write(uint32_t addr, uint16_t data) const {
    if (_devs->isSelected(addr)) {
        _devs->write(addr, data);
    } else {
        write_byte(addr, data);
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
