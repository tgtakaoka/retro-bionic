#include "identity.h"

#include "pins_z180.h"

namespace debugger {
namespace hd64180s {

// Its internal I/O is not the HD64180's: 32H and 36H are not DCNTL and
// RCR. Zeroing 0AH (memory waits) and 18H (refresh, 80H at reset) runs
// a loop in a third of the time, as fast as the Z180's setup does; found
// by timing a loop after zeroing each port, with no register map at hand.
constexpr uint8_t CONFIG[] = {
        0xAF,              // XOR A
        0xED, 0x39, 0x0A,  // OUT0 (0AH), A
        0xED, 0x39, 0x18,  // OUT0 (18H), A
        0x18, 0xF7         // JR $-7
};

Pins *instance() {
    return new z180::PinsZ180(CONFIG, sizeof(CONFIG));
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
