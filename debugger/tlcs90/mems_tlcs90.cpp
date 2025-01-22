#include "mems_tlcs90.h"
#include <asm_tlcs90.h>
#include <dis_tlcs90.h>
#include "regs_tlcs90.h"

namespace debugger {
namespace tlcs90 {

MemsTlcs90::MemsTlcs90(RegsTlcs90 *regs, Devs *devs)
    : DmaMemory(Endian::ENDIAN_LITTLE), _regs(regs), _devs(devs) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::tlcs90::AsmTlcs90();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::tlcs90::DisTlcs90();
#endif
}

uint16_t MemsTlcs90::read(uint32_t addr) const {
    return _devs->isSelected(addr) ? _devs->read(addr) : read_byte(addr);
}

void MemsTlcs90::write(uint32_t addr, uint16_t data) const {
    if (_devs->isSelected(addr)) {
        _devs->write(addr, data);
    } else {
        write_byte(addr, data);
    }
}

uint16_t MemsTlcs90::get(uint32_t addr, const char *) const {
    return is_internal(addr) ? _regs->internal_read(addr) : read(addr);
}

void MemsTlcs90::put(uint32_t addr, uint16_t data, const char *) const {
    if (is_internal(addr)) {
        _regs->internal_write(addr, data);
    } else {
        write(addr, data);
    }
}

}  // namespace tlcs90
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
