#ifndef __Z88_UART_HANDLER_H__
#define __Z88_UART_HANDLER_H__

#include "serial_handler.h"

namespace debugger {
namespace z88 {
struct Z88UartHandler final : SerialHandler {
    Z88UartHandler();

    const char *name() const override;
    const char *description() const override;
    uint32_t baseAddr() const override;

protected:
    void resetHandler() override;
};

extern struct Z88UartHandler UartH;

}  // namespace z88
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
