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

}  // namespace cdp1802
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
