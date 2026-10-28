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
    // ATTENTION: 74 x 14 is 1036 XIN a bit, which matches neither 2 MHz
    // nor 4 MHz at 4800 bps; INS8073 BASIC echoes stray 0xFF and loses
    // input. Measure the CPU's bit time on F1 and retune.
    // baudrate 4800 bps
    _pre_divider = 74;
    _tx_divider = _rx_divider = 14;
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
