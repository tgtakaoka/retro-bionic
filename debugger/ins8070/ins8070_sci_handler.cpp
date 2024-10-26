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
    // INS8070 bitbang speed: assuming XTAL is 2MHz
    // baudrate 4800 bps
    _pre_divider = 74;
    _divider = 14;
    // baudrate 110 bps
    // _pre_divider = 2561;
    // _divider = 18;
}

}  // namespace ins8070
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
