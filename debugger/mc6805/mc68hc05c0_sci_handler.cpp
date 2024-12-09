#include "mc68hc05c0_sci_handler.h"
#include "debugger.h"
#include "pins_mc68hc05c0.h"

namespace debugger {
namespace mc68hc05c0 {

const char *Mc68HC05C0SciHandler::name() const {
    return "SCI";
}

const char *Mc68HC05C0SciHandler::description() const {
    return Debugger.target().cpuName();
}

bool Mc68HC05C0SciHandler::isSelected(uint32_t addr) const {
    return addr == ADDR_SCBR;
}

void Mc68HC05C0SciHandler::write(uint32_t addr, uint16_t data) {
    _scbr = data;
    resetHandler();
}

void Mc68HC05C0SciHandler::assert_rxd() const {
    digitalWriteFast(PIN_RDI, LOW);
}

void Mc68HC05C0SciHandler::negate_rxd() const {
    digitalWriteFast(PIN_RDI, HIGH);
}

uint8_t Mc68HC05C0SciHandler::signal_txd() const {
    return digitalReadFast(PIN_TDO);
}

void Mc68HC05C0SciHandler::resetHandler() {
    pinMode(PIN_RDI, OUTPUT);
    pinMode(PIN_TDO, INPUT);
    // Baud rate = fOSC/4 / SCP / SCT,SCR / 16
    static const uint8_t scaler[] = {1, 3, 4, 13};
    _pre_divider = scaler[(_scbr & SCP_gm) >> SCP_gp];
    _tx_divider = 16 << ((_scbr & SCT_gm) >> SCT_gp);
    _rx_divider = 16 << ((_scbr & SCR_gm) >> SCR_gp);
}

}  // namespace mc68hc05c0
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
