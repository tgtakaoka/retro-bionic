#include "ins8060_sci_handler.h"
#include "pins_ins8060.h"
#include "regs_ins8060.h"

namespace debugger {
namespace ins8060 {

// PIN_FLAG0 is inverted
Ins8060SciHandler::Ins8060SciHandler()
    : SerialHandler(PIN_SENSEB, PIN_FLAG0, false, true) {}

const char *Ins8060SciHandler::name() const {
    return "BitBang";
}

const char *Ins8060SciHandler::description() const {
    return Regs.cpuName();
}

void Ins8060SciHandler::resetHandler() {
    // INS8060 bitbang speed: assuming XTAL is 2MHz
    // baudrate 1200 bps
    _pre_divider = 119;
    _divider = 14;
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
