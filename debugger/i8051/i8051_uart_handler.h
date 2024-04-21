#ifndef __I8051_UART_HANDLER_H__
#define __I8051_UART_HANDLER_H__

#include "serial_handler.h"

namespace debugger {
namespace i8051 {

struct I8051UartHandler final : SerialHandler {
    I8051UartHandler();

    const char *name() const override;
    const char *description() const override;
    uint32_t baseAddr() const override;

protected:
    void resetHandler() override;
};

extern struct I8051UartHandler UartH;

}  // namespace i8051
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
