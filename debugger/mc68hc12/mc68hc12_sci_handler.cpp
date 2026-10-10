#include "mc68hc12_sci_handler.h"
#include "debugger.h"
#include "mc68hc12_init.h"
#include "pins_mc68hc12.h"

namespace debugger {
namespace mc68hc12 {

const char *Mc68hc12SciHandler::name() const {
    return "SCI";
}

const char *Mc68hc12SciHandler::description() const {
    return Debugger.target().cpuName();
}

uint32_t Mc68hc12SciHandler::baseAddr() const {
    return Mc68hc12Init::REG_BASE + Mc68hc12Init::SC0BDH;
}

bool Mc68hc12SciHandler::isSelected(uint32_t addr) const {
    return addr - baseAddr() < 2;
}

void Mc68hc12SciHandler::write(uint32_t addr, uint16_t data) {
    if (addr == baseAddr()) {
        _sbr = ((data & 0x1F) << 8) | (_sbr & 0xFF);
    } else {
        _sbr = (_sbr & 0x1F00) | (data & 0xFF);
    }
    resetHandler();
}

void Mc68hc12SciHandler::assert_rxd() const {
    digitalWriteFast(PIN_RXD, LOW);
}

void Mc68hc12SciHandler::negate_rxd() const {
    digitalWriteFast(PIN_RXD, HIGH);
}

uint8_t Mc68hc12SciHandler::signal_txd() const {
    return digitalReadFast(PIN_TXD);
}

void Mc68hc12SciHandler::resetHandler() {
    pinMode(PIN_RXD, OUTPUT);
    pinMode(PIN_TXD, INPUT);
    // Baud rate = E / (16 * SBR); loop() runs once a bus cycle.
    _pre_divider = _sbr ? _sbr : 1;
    _tx_divider = _rx_divider = 16;
}

}  // namespace mc68hc12
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
