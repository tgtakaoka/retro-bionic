#ifndef __TLCS90_UART_HANDLER_H__
#define __TLCS90_UART_HANDLER_H__

#include "serial_handler.h"

namespace debugger {
namespace tlcs90 {

struct Tlcs90UartHandler final : SerialHandler {
    const char *name() const override;
    const char *description() const override;
    uint32_t baseAddr() const override;

protected:
    void resetHandler() override;
    void assert_rxd() const override;
    void negate_rxd() const override;
    uint8_t signal_txd() const override;
};

}  // namespace tlcs90
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
