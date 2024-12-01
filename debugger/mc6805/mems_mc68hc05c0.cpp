#include "mems_mc68hc05c0.h"
#include <asm_mc6805.h>
#include <dis_mc6805.h>

namespace debugger {
namespace mc68hc05c0 {

bool MemsMc68HC05C0::is_internal(uint16_t addr) const {
    if (addr < 0x40) {
        static constexpr uint8_t IO_MAP[] = {
                0x88,  // $00-$07
                0x00,  // $08-$0F
                0x00,  // $10-$17
                0x0F,  // $18-$1F
                0xFF,  // $20-$27
                0xFF,  // $28-$2F
                0xFF,  // $30-$37
                0xFF,  // $38-$3F
        };
        return (IO_MAP[addr >> 3] & (0x80 >> (addr & 7))) == 0;
    }
    return addr < 0x240;
}

}  // namespace mc68hc05c0
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
