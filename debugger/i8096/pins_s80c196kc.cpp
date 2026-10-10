#include "pins_s80c196kc.h"
#include "debugger.h"
#include "signals_s80c196kc.h"

namespace debugger {
namespace s80c196kc {

void PinsS80C196KC::resetPins() {
    pinMode(PIN_INST, INPUT);
    // A rising NMI vectors to 203EH.
    pinMode(PIN_NMI, OUTPUT);
    digitalWriteFast(PIN_NMI, LOW);
    // #HOLD shares P1.7, a quasi-bidirectional port pin: a pull-up keeps
    // it negated without fighting a program that drives the port low.
    pinMode(PIN_HOLD, INPUT_PULLUP);
    pinMode(PIN_HLDA, INPUT);
    PinsI8096::resetPins();
}

bool PinsS80C196KC::getControl(i8096::SignalsI8096 *s) const {
    return static_cast<s80c196kc::Signals *>(s)->getControl();
}

}  // namespace s80c196kc
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
