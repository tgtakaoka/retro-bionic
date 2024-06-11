#ifndef __MC68HC11_SCI_HANDLER_H__
#define __MC68HC11_SCI_HANDLER_H__

#include "serial_handler.h"

namespace debugger {
namespace mc68hc11 {

struct Mc68hc11Init;

struct Mc68hc11SciHandler final : SerialHandler {
    Mc68hc11SciHandler(const Mc68hc11Init &init)
        : SerialHandler(), _init(init), _baud(0) {}

    const char *name() const override;
    const char *description() const override;
    uint32_t baseAddr() const override;
    void write(uint32_t addr, uint16_t data) override;

protected:
    const Mc68hc11Init &_init;
    uint8_t _baud;  // BAUD(+$2B): SCI Baudrate control register

    void resetHandler() override;
    void assert_rxd() const override;
    void negate_rxd() const override;
    uint8_t signal_txd() const override;

    static constexpr uint16_t BAUD = 0x2B;
    static constexpr uint8_t SCR_gp = 0;
    static constexpr uint8_t SCR_gm = 7;
    static constexpr uint8_t SCP_gp = 4;
    static constexpr uint8_t SCP_gm = 3;
};

}  // namespace mc68hc11
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
