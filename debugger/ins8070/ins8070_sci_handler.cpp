#include "ins8070_sci_handler.h"
#include "pins_ins8070.h"
#include "regs_ins8070.h"

namespace debugger {
namespace ins8070 {

// PIN_F1 is inverted
Ins8070SciHandler::Ins8070SciHandler()
    : SerialHandler(PIN_SA, PIN_F1, false, true) {}

const char *Ins8070SciHandler::name() const {
    return "BitBang";
}

const char *Ins8070SciHandler::description() const {
    return Regs.cpuName();
}

void Ins8070SciHandler::resetHandler() {
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
