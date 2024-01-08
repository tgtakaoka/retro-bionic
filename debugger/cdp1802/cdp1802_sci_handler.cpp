#include "cdp1802_sci_handler.h"
#include "pins_cdp1802.h"

namespace debugger {
namespace cdp1802 {

Cdp1802SciHandler::Cdp1802SciHandler() : SerialHandler(PIN_EF3, PIN_Q) {}

const char *Cdp1802SciHandler::name() const {
    return "BitBang";
}

const char *Cdp1802SciHandler::description() const {
    return "Q/#EF3";
}

uint32_t Cdp1802SciHandler::baseAddr() const {
    return 0;
}

void Cdp1802SciHandler::resetHandler() {
    // CDP1802 Xtal: 1.79MHz, CPU clock 223.75kHz
    // Bitbang speed: 9600bps
    _pre_divider = 1;
    _divider = 23;
}

}  // namespace cdp1802
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
