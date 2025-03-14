#include "ins8060_sci_handler.h"
#include "debugger.h"
#include "pins_ins8060.h"

namespace debugger {
namespace ins8060 {

const char *Ins8060SciHandler::name() const {
    return "BitBang";
}

const char *Ins8060SciHandler::description() const {
    return Debugger.target().cpuName();
}

void Ins8060SciHandler::assert_rxd() const {
    digitalWriteFast(PIN_SENSEB, LOW);
}

void Ins8060SciHandler::negate_rxd() const {
    digitalWriteFast(PIN_SENSEB, HIGH);
}

uint8_t Ins8060SciHandler::signal_txd() const {
    // PIN_FLAG0 is inverted
    return HIGH - digitalReadFast(PIN_FLAG0);
}

void Ins8060SciHandler::resetHandler() {
    pinMode(PIN_SENSEB, OUTPUT);
    pinMode(PIN_FLAG0, INPUT);
    // INS8060 bitbang speed: assuming XTAL is 2MHz
    // baudrate 1200 bps
    _pre_divider = 119;
    _tx_divider = _rx_divider = 14;
    // baudrate 110 bps
    // _pre_divider = 1010;
    // _divider = 18;
}

}  // namespace ins8060
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
