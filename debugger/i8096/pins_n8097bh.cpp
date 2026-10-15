#include "pins_n8097bh.h"
#include "debugger.h"
#include "signals_n8097bh.h"

namespace debugger {
namespace n8097bh {

void PinsN8097BH::resetPins() {
    pinMode(PIN_INST, INPUT);
    pinMode(PIN_ACH5, INPUT);
    pinMode(PIN_ACH6, INPUT);
    // A rising NMI vectors to 0000H.
    pinMode(PIN_NMI, OUTPUT);
    digitalWriteFast(PIN_NMI, LOW);
    PinsI8096::resetPins();
}

void PinsN8097BH::assertInt(uint8_t name) {
    if (name == i8096::INTR_NMI) {
        digitalWriteFast(PIN_NMI, HIGH);
    } else {
        PinsI8096::assertInt(name);
    }
}

void PinsN8097BH::negateInt(uint8_t name) {
    if (name == i8096::INTR_NMI) {
        digitalWriteFast(PIN_NMI, LOW);
    } else {
        PinsI8096::negateInt(name);
    }
}

bool PinsN8097BH::getControl(i8096::SignalsI8096 *s) const {
    return static_cast<n8097bh::Signals *>(s)->getControl();
}

}  // namespace n8097bh
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
