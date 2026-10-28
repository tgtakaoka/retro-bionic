#include "ins8070_sci_handler.h"
#include "debugger.h"
#include "pins_ins8070.h"

namespace debugger {
namespace ins8070 {

const char *Ins8070SciHandler::name() const {
    return "BitBang";
}

const char *Ins8070SciHandler::description() const {
    return Debugger.target().cpuName();
}

void Ins8070SciHandler::assert_rxd() const {
    digitalWriteFast(PIN_SA, LOW);
}

void Ins8070SciHandler::negate_rxd() const {
    digitalWriteFast(PIN_SA, HIGH);
}

uint8_t Ins8070SciHandler::signal_txd() const {
    // PIN_F1 is inverted
    return HIGH - digitalReadFast(PIN_F1);
}

void Ins8070SciHandler::resetHandler() {
    pinMode(PIN_SA, OUTPUT);
    pinMode(PIN_F1, INPUT);
    // INS8073 BASIC at 4800 bps, measured on F1: it sends a bit every
    // 1012 XIN, and receives, echoing each bit, one every 1078, as its
    // delay loops wait out the bus cycles the Teensy holds too. Rx sits
    // between the two.
    _pre_divider = 37;
    _rx_divider = 28;
    _tx_divider = 29;
    // baudrate 110 bps
    // _pre_divider = 2561;
    // _tx_divider = _rx_divider = 18;
}

}  // namespace ins8070
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
