#include "mems_tms9900.h"
#include <asm_tms9900.h>
#include <dis_tms9900.h>

namespace debugger {
namespace tms9900 {

MemsTms9900::MemsTms9900(uint8_t addr_bits, Devs *devs)
    : DmaMemory(Endian::ENDIAN_BIG),
      _max_addr((UINT32_C(1) << addr_bits) - 1),
      _devs(devs) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::tms9900::AsmTms9900();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::tms9900::DisTms9900();
#endif
}

uint16_t MemsTms9900::read(uint32_t addr) const {
    return _devs->isSelected(addr) ? _devs->read(addr) : raw_read(addr);
}

void MemsTms9900::write(uint32_t addr, uint16_t data) const {
    if (_devs->isSelected(addr)) {
        _devs->write(addr, data);
    } else {
        raw_write(addr, data);
    }
}

uint16_t MemsTms9900::get(uint32_t addr, const char *space) const {
    return _devs->isSelected(addr) ? _devs->read(addr) : raw_read(addr);
}

void MemsTms9900::put(uint32_t addr, uint16_t data, const char *space) const {
    if (_devs->isSelected(addr)) {
        _devs->write(addr, data);
    } else {
        raw_write(addr, data);
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