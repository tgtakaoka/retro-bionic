#include "identity.h"

#include "pins_z180.h"

namespace debugger {
namespace hd64180s {

// Its internal I/O is not the HD64180's: 32H and 36H are not DCNTL and
// RCR. Zeroing WCRL (0AH, memory waits) and RCR (18H, refresh, 80H at
// reset) runs a loop in a third of the time, as fast as the Z180's setup
// does. The names are from Hitachi's NPU initialization in the HD64180
// Family Application Handbook, which zeroes 0AH-0FH and RCR; its scan
// reads RCR as 1BH, but 18H is the port timing found.
constexpr uint8_t CONFIG[] = {
        0xAF,              // XOR A
        0xED, 0x39, 0x0A,  // OUT0 (WCRL), A
        0xED, 0x39, 0x18,  // OUT0 (RCR), A
        0x18, 0xF7         // JR $-7
};

Pins *instance() {
    // HD64180SH10: EXTAL up to 20 MHz; internal I/O at 0000-0087H; #WAIT
    // holds a cycle before its strobes.
    return new z180::PinsZ180(CONFIG, sizeof(CONFIG), 18750000, 0x88, true);
}

TARGET_OVERLAY(z80);

const struct Identity HD64180S{"HD64180S", instance, overlay};

}  // namespace hd64180s
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
