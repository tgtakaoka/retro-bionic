#include "mems_tms9900.h"
#include <asm_tms9900.h>
#include <dis_tms9900.h>

namespace debugger {
namespace tms9900 {

MemsTms9900::MemsTms9900(Devs *devs)
    : DmaMemory(Endian::ENDIAN_BIG), _devs(devs) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::tms9900::AsmTms9900();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::tms9900::DisTms9900();
#endif
}

uint16_t MemsTms9900::read(uint32_t addr) const {
    return _devs->isSelected(addr) ? uint16(_devs->read(addr), 0)
                                   : read_word(addr / 2);
}

void MemsTms9900::write(uint32_t addr, uint16_t data) const {
    if (_devs->isSelected(addr)) {
        _devs->write(addr, hi(data));
    } else {
        write_word(addr / 2, data);
    }
}

}  // namespace tms9900
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
