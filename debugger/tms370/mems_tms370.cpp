#include "mems_tms370.h"
#include <asm_tms370.h>
#include <dis_tms370.h>
#include "pins_tms370.h"
#include "regs_tms370.h"

namespace debugger {
namespace tms370 {

MemsTms370::MemsTms370(RegsTms370 *regs, PinsTms370 *pins, Devs *devs)
    : DmaMemory(Endian::ENDIAN_BIG), _regs(regs), _pins(pins), _devs(devs) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::tms370::AsmTms370();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::tms370::DisTms370();

#endif
}

uint16_t MemsTms370::read(uint32_t addr) const {
    return _devs->isSelected(addr) ? _devs->read(addr) : read_byte(addr);
}

void MemsTms370::write(uint32_t addr, uint16_t data) const {
    if (_devs->isSelected(addr)) {
        _devs->write(addr, data);
    } else {
        write_byte(addr, data);
    }
}

uint16_t MemsTms370::get_data(uint32_t addr) const {
    return internal(addr) ? _regs->read_internal(addr) : read(addr);
}

void MemsTms370::put_data(uint32_t addr, uint16_t data) const {
    if (internal(addr)) {
        _regs->write_internal(addr, data);
    } else {
        write(addr, data);
    }
}

}  // namespace tms370
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
