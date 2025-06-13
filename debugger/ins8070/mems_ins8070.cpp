#include "mems_ins8070.h"
#include <asm_ins8070.h>
#include <dis_ins8070.h>
#include "devs_ins8070.h"
#include "regs_ins8070.h"

namespace debugger {
namespace ins8070 {

MemsIns8070::MemsIns8070(RegsIns8070 *regs, Devs *devs)
    : DmaMemory(Endian::ENDIAN_LITTLE), _regs(regs), _devs(devs) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::ins8070::AsmIns8070();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::ins8070::DisIns8070();
#endif
}

uint16_t MemsIns8070::read(uint32_t addr) const {
    return _devs->isSelected(addr) ? _devs->read(addr) : read_byte(addr);
}

void MemsIns8070::write(uint32_t addr, uint16_t data) const {
    if (_devs->isSelected(addr)) {
        _devs->write(addr, data);
    } else {
        write_byte(addr, data);
    }
}

uint16_t MemsIns8070::get_data(uint32_t addr) const {
    return is_internal(addr) ? _regs->internal_read(addr) : read(addr);
}

void MemsIns8070::put_data(uint32_t addr, uint16_t data) const {
    if (is_internal(addr)) {
        _regs->internal_write(addr, data);
    } else {
        write(addr, data);
    }
}

}  // namespace ins8070
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
