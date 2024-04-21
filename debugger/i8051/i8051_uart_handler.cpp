#include "i8051_uart_handler.h"
#include "pins_i8051.h"
#include "regs_i8051.h"

namespace debugger {
namespace i8051 {

I8051UartHandler::I8051UartHandler() : SerialHandler(PIN_RXD, PIN_TXD) {}

const char *I8051UartHandler::name() const {
    return "UART";
}

const char *I8051UartHandler::description() const {
    return Regs.cpuName();
}

uint32_t I8051UartHandler::baseAddr() const {
    return 0x0099;  // SBUF
}

void I8051UartHandler::resetHandler() {
    // I8051 UART:
    // baudrate = K*fosc/(32*12*(256-TH1)
    // 256-TH1 = K*fosc/(32*12*baudrate)
    // fosc=12MHz, K=2(SMOD=1) baudrate=4,800bps, 256-TH1=13
    _pre_divider = 12 * 13 * 2;
    _divider = 16;
}

}  // namespace i8051
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
