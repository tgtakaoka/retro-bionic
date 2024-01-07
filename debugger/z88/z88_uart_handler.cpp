#include "z88_uart_handler.h"
#include "pins_z88.h"
#include "regs_z88.h"

namespace debugger {
namespace z88 {

Z88UartHandler::Z88UartHandler() : SerialHandler(PIN_RXD, PIN_TXD) {}

const char *Z88UartHandler::name() const {
    return "UART";
}

const char *Z88UartHandler::description() const {
    return Regs.cpuName();
}

uint32_t Z88UartHandler::baseAddr() const {
    return 0xEF;
}

void Z88UartHandler::resetHandler() {
    // Z88 UART: assuming XTAL is 14.7546MHz
    // bit rate = (14,754,600 / 4) / (2 x (UBG+1) x N)
    _pre_divider = 4 * 2 * 32;  // N=32
    _divider = 11 + 1;          // UBG=11
}

}  // namespace z88
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
