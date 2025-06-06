#include "mems_cdp1802.h"
#include <asm_cdp1802.h>
#include <dis_cdp1802.h>

namespace debugger {
namespace cdp1802 {

MemsCdp1802::MemsCdp1802() : DmaMemory(Endian::ENDIAN_BIG) {
#ifdef WITH_ASSEMBLER
    _assembler = new libasm::cdp1802::AsmCdp1802();
#endif
#ifdef WITH_DISASSEMBLER
    _disassembler = new libasm::cdp1802::DisCdp1802();
#endif
}

uint16_t MemsCdp1802::read(uint32_t addr) const {
    return read_byte(addr);
}

void MemsCdp1802::write(uint32_t addr, uint16_t data) const {
    write_byte(addr, data);
}

}  // namespace cdp1802
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
