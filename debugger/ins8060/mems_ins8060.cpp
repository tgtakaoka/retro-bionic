#include <asm_ins8060.h>
#include <dis_ins8060.h>

#include "mems_ins8060.h"

namespace debugger {
namespace ins8060 {

MemsIns8060::MemsIns8060(Devs *devs)
    : DmaMemory(Endian::ENDIAN_LITTLE), _devs(devs) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::ins8060::AsmIns8060();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::ins8060::DisIns8060();
#endif
}

uint16_t MemsIns8060::read(uint32_t addr) const {
    if (_devs->isSelected(addr))
        return _devs->read(addr);
    return read_byte(addr);
}

void MemsIns8060::write(uint32_t addr, uint16_t data) const {
    if (_devs->isSelected(addr)) {
        _devs->write(addr, data);
    } else {
        write_byte(addr, data);
    }
}
}  // namespace ins8060
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
