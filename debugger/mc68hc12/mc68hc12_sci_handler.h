#ifndef __MC68HC12_SCI_HANDLER_H__
#define __MC68HC12_SCI_HANDLER_H__

#include "serial_handler.h"

namespace debugger {
namespace mc68hc12 {

// SCI0, bit-banged on PS0/PS1. The CPU's writes to SC0BDH/SC0BDL show on
// the bus (IVIS), so the baud rate divisor is snooped there.
struct Mc68hc12SciHandler final : SerialHandler {
    Mc68hc12SciHandler() : SerialHandler(), _sbr(0) {}

    const char *name() const override;
    const char *description() const override;
    uint32_t baseAddr() const override;
    bool isSelected(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) override;

protected:
    uint16_t _sbr;  // SC0BDH:SC0BDL, SBR12:0

    void resetHandler() override;
    void assert_rxd() const override;
    void negate_rxd() const override;
    uint8_t signal_txd() const override;
};

}  // namespace mc68hc12
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
