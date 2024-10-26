#include "mc68hc11_sci_handler.h"
#include "debugger.h"
#include "mc68hc11_init.h"
#include "pins_mc68hc11.h"

namespace debugger {
namespace mc68hc11 {

const char *Mc68hc11SciHandler::name() const {
    return "SCI";
}

const char *Mc68hc11SciHandler::description() const {
    return Debugger.target().cpuName();
}

uint32_t Mc68hc11SciHandler::baseAddr() const {
    return _init.dev_base() + BAUD;
}

void Mc68hc11SciHandler::write(uint32_t addr, uint16_t data) {
    _baud = data;
    resetHandler();
}

void Mc68hc11SciHandler::assert_rxd() const {
    digitalWriteFast(PIN_SCIRXD, LOW);
}

void Mc68hc11SciHandler::negate_rxd() const {
    digitalWriteFast(PIN_SCIRXD, HIGH);
}

uint8_t Mc68hc11SciHandler::signal_txd() const {
    return digitalReadFast(PIN_SCITXD);
}

void Mc68hc11SciHandler::resetHandler() {
    pinMode(PIN_SCIRXD, OUTPUT);
    pinMode(PIN_SCITXD, INPUT);
    static const uint16_t scaler[] = {1, 3, 4, 13};
    static const uint16_t selector[] = {1, 2, 4, 8, 16, 32, 64, 128};
    const auto scp = (_baud >> SCP_gp) & SCP_gm;
    const auto scr = (_baud >> SCR_gp) & SCR_gm;
    _pre_divider = scaler[scp] * selector[scr];
    _divider = 16;
}

}  // namespace mc68hc11
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
