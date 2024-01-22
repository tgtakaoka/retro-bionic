#ifndef __INS8060_SCI_HANDLER_H__
#define __INS8060_SCI_HANDLER_H__

#include "serial_handler.h"

namespace debugger {
namespace ins8060 {

struct Ins8060SciHandler final : SerialHandler {
    Ins8060SciHandler();

    const char *name() const override;
    const char *description() const override;
    uint32_t baseAddr() const override { return 0; }

protected:
    void resetHandler() override;
};

extern struct Ins8060SciHandler SciH;

}  // namespace ins8060
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
