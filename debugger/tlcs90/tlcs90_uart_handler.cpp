#include "tlcs90_uart_handler.h"
#include "pins_tlcs90.h"
#include "regs_tlcs90.h"

namespace debugger {
namespace tlcs90 {

Tlcs90UartHandler::Tlcs90UartHandler() : SerialHandler(PIN_RXD, PIN_TXD) {}

const char *Tlcs90UartHandler::name() const {
    return "UART";
}

const char *Tlcs90UartHandler::description() const {
    return Regs.cpuName();
}

uint32_t Tlcs90UartHandler::baseAddr() const {
    return 0xFFEB;
}

void Tlcs90UartHandler::resetHandler() {
    // TLCS90 UART: assuming XTAL is fc=9.8304MHz
    // bitrate=fc/4/Tn/SC/16
    // TRUN_BRATE=11, T4 clock, 9600bps
    // SCMOD_SC=11, 1/2 division
    _pre_divider = 4 * 4 * 2;
    _divider = 16;
}

}  // namespace tlcs90
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
