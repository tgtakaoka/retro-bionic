#include "tms7002_serial_handler.h"
#include "debugger.h"
#include "pins_tms7000.h"

namespace debugger {
namespace tms7002 {

struct Tms7002SerialHandler SerialH;

const char *Tms7002SerialHandler::name() const {
    return "Serial";
}

const char *Tms7002SerialHandler::description() const {
    return Debugger.target().cpuName();
}

uint32_t Tms7002SerialHandler::baseAddr() const {
    return 0x0111;  // SMODE/SCTL0/SSTAT
}

void Tms7002SerialHandler::assert_rxd() const {
    digitalWriteFast(PIN_PA5, LOW);
}

void Tms7002SerialHandler::negate_rxd() const {
    digitalWriteFast(PIN_PA5, HIGH);
}

uint8_t Tms7002SerialHandler::signal_txd() const {
    return digitalReadFast(PIN_PB3);
}

void Tms7002SerialHandler::resetHandler() {
    pinMode(PIN_PA5, OUTPUT);
    pinMode(PIN_PB3, INPUT);
    // TMS7002/TMS7001 Serial:
    // fosc: XTAL frequency
    // CLK: system clock; fosc/2 (/2 clock option), fosc/4 (/4 clock option)
    // PR: Timer 3 prescaler reload value
    // TR: TImer 3 reload value
    // Serial Clock (SCLK) = (CLK / 2) / (PR + 1) / (TR + 1) / 2
    // Asynchronous baud rate; SCLK/8 (TMS7002), SCLK/16 (TMS7001)
    constexpr auto PR = 0;
    constexpr auto TR = 12;
    _pre_divider = (PR + 1) * (TR + 1) * 4 * 2;
    _divider = _tms7001 ? 16 : 8;
}

}  // namespace tms7002
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
