#include "cdp1802_sci_handler.h"
#include "debugger.h"
#include "pins_cdp1802.h"

namespace debugger {
namespace cdp1802 {

const char *Cdp1802SciHandler::name() const {
    return "BitBang";
}

const char *Cdp1802SciHandler::description() const {
    return "Q/#EF3";
}

uint32_t Cdp1802SciHandler::baseAddr() const {
    return 0;
}

void Cdp1802SciHandler::assert_rxd() const {
    digitalWriteFast(PIN_EF3, LOW);
}

void Cdp1802SciHandler::negate_rxd() const {
    digitalWriteFast(PIN_EF3, HIGH);
}

uint8_t Cdp1802SciHandler::signal_txd() const {
    return digitalReadFast(PIN_Q);
}

void Cdp1802SciHandler::resetHandler() {
    pinMode(PIN_EF3, OUTPUT);
    pinMode(PIN_Q, INPUT);
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
