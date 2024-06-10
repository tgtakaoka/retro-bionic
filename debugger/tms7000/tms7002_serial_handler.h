#ifndef __TMS7002_SERIAL_HANDLER_H__
#define __TMS7002_SERIAL_HANDLER_H__

#include "serial_handler.h"

namespace debugger {
namespace tms7002 {

struct Tms7002SerialHandler final : SerialHandler {
    Tms7002SerialHandler() : SerialHandler(), _tms7001(false) {}

    const char *name() const override;
    const char *description() const override;
    uint32_t baseAddr() const override;

    void setTms7001(bool tms7001) { _tms7001 = tms7001; }

protected:
    void resetHandler() override;
    void assert_rxd() const override;
    void negate_rxd() const override;
    uint8_t signal_txd() const override;

private:
    bool _tms7001;
};

extern struct Tms7002SerialHandler SerialH;

}  // namespace tms7002
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
