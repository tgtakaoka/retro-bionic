#ifndef __I8085_SIO_HANDLER_H__
#define __I8085_SIO_HANDLER_H__

#include "serial_handler.h"

namespace debugger {
namespace i8085 {

struct I8085SioHandler final : SerialHandler {
    I8085SioHandler();

    const char *name() const override;
    const char *description() const override;
    uint32_t baseAddr() const override;

protected:
    void resetHandler() override;
};

extern struct I8085SioHandler SioH;

}  // namespace i8085
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
