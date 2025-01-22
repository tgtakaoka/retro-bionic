#include "mems_tms7000.h"
#include <asm_tms7000.h>
#include <ctype.h>
#include <dis_tms7000.h>
#include "regs_tms7000.h"

namespace debugger {
namespace tms7000 {

MemsTms7000::MemsTms7000(RegsTms7000 *regs, Devs *devs)
    : DmaMemory(Endian::ENDIAN_BIG), _regs(regs), _devs(devs) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::tms7000::AsmTms7000();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::tms7000::DisTms7000();

#endif
}

uint16_t MemsTms7000::read(uint32_t addr) const {
    return _devs->isSelected(addr) ? _devs->read(addr) : read_byte(addr);
}

void MemsTms7000::write(uint32_t addr, uint16_t data) const {
    if (_devs->isSelected(addr)) {
        _devs->write(addr, data);
    } else {
        write_byte(addr, data);
    }
}

uint16_t MemsTms7000::get(uint32_t addr, const char *space) const {
    if (_devs->isSelected(addr))
        return _devs->read(addr);
    return addr < 0x0200 ? _regs->read_internal(addr) : read_byte(addr);
}

void MemsTms7000::put(uint32_t addr, uint16_t data, const char *space) const {
    if (_devs->isSelected(addr)) {
        _devs->write(addr, data);
    } else if (addr < 0x0200) {
        _regs->write_internal(addr, data);
    } else {
        write_byte(addr, data);
    }
}
}  // namespace tms7000
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
