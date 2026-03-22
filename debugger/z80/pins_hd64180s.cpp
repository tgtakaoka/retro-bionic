#include "pins_hd64180s.h"

namespace debugger {
namespace hd64180s {

void PinsHD64180S::configureCpu() {
    // Set 0-wait for memory and 4-wait for I/O (DCNTL)
    // Disable DRAM refresh (RCR)
    static constexpr uint8_t CONFIG[] = {
            0x3E, 0x10,        // LD A, 10H
            0xED, 0x39, 0x32,  // OUT0 (DCNTL), A
            0xAF,              // XOR A
            0xED, 0x39, 0x36,  // OUT0 (RCR), A
            0x18, 0xF5,        // JR $-9
    };
    execInst(CONFIG, sizeof(CONFIG));
}

}  // namespace hd64180s
}  // namespace debugger
// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
