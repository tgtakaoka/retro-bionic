#ifndef __MC68HC05C0_SCI_HANDLER_H__
#define __MC68HC05C0_SCI_HANDLER_H__

#include "serial_handler.h"

namespace debugger {
namespace mc68hc05c0 {

struct Mc68HC05C0SciHandler final : SerialHandler {
    Mc68HC05C0SciHandler() : _scbr(0x3F) {}

    const char *name() const override;
    const char *description() const override;
    uint32_t baseAddr() const override { return ADDR_SCBR; }
    bool isSelected(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) override;

private:
    uint8_t _scbr;  // SCBR($14) Baud Rate Register
    static constexpr uint16_t ADDR_SCBR = 0x0014;
    static constexpr uint8_t SCP_gm = 0xC0;  // Prescaler
    static constexpr uint8_t SCT_gm = 0x38;  // Transmit Baud Rate Selection
    static constexpr uint8_t SCR_gm = 0x07;  // Receiver Baud Rate Selection
    static constexpr auto SCP_gp = 6;
    static constexpr auto SCT_gp = 3;
    static constexpr auto SCR_gp = 0;

    void resetHandler() override;
    void assert_rxd() const override;
    void negate_rxd() const override;
    uint8_t signal_txd() const override;
    void setScbr(uint8_t);
};

}  // namespace mc68hc05c0
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
