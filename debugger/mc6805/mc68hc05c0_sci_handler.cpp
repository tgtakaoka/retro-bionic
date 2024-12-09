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
    setScbr(_scbr = data);
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
    setScbr(_scbr);
}

void Mc68HC05C0SciHandler::setScbr(uint8_t scbr) {
    // Baud rate = fOSC/4 / SCP / SCT,SCR / 16
    static const uint8_t scaler[] = {1, 3, 4, 13};
    _pre_divider = scaler[(scbr & SCP_gm) >> SCP_gp] * 16;
    _tx_divider = 1 << ((scbr & SCT_gm) >> SCT_gp);
    _rx_divider = 1 << ((scbr & SCR_gm) >> SCR_gp);
}

}  // namespace mc68hc05c0
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
