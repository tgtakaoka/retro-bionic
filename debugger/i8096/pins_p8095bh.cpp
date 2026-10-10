#include "pins_p8095bh.h"
#include "debugger.h"
#include "signals_p8095bh.h"

namespace debugger {
namespace p8095bh {

void PinsP8095BH::resetPins() {
    pinMode(PIN_PWM, INPUT);
    pinMode(PIN_ACH4, INPUT);
    pinMode(PIN_ACH5, INPUT);
    pinMode(PIN_ACH6, INPUT);
    PinsI8096::resetPins();
}

bool PinsP8095BH::getControl(i8096::SignalsI8096 *s) const {
    return static_cast<p8095bh::Signals *>(s)->getControl();
}

}  // namespace p8095bh
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
