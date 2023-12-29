#ifndef __TLCS90_UART_HANDLER_H__
#define __TLCS90_UART_HANDLER_H__

#include "serial_handler.h"

namespace debugger {
namespace tlcs90 {

struct Tlcs90UartHandler final : SerialHandler {
    Tlcs90UartHandler();

    const char *name() const override;
    const char *description() const override;
    uint32_t baseAddr() const override;

protected:
    void resetHandler() override;
};

extern struct Tlcs90UartHandler UartH;

}  // namespace tlcs90
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
