#include "pins_n8097bh.h"
#include "debugger.h"
#include "signals_n8097bh.h"

namespace debugger {
namespace n8097bh {

void PinsN8097BH::resetPins() {
    pinMode(PIN_INST, INPUT);
    // A rising NMI vectors to 0000H.
    pinMode(PIN_NMI, OUTPUT);
    digitalWriteFast(PIN_NMI, LOW);
    PinsI8096::resetPins();
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
