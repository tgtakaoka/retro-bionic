#ifndef __Z86_SIO_HANDLER_H__
#define __Z86_SIO_HANDLER_H__

#include "serial_handler.h"

namespace debugger {
namespace z86 {
struct Z86SioHandler final : SerialHandler {
    Z86SioHandler();

    const char *name() const override;
    const char *description() const override;
    uint32_t baseAddr() const override;

protected:
    void resetHandler() override;
};

extern struct Z86SioHandler SioH;

}  // namespace z86
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
