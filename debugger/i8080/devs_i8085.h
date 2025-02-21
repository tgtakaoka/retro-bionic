#ifndef __DEVS_I8085_H__
#define __DEVS_I8085_H__

#include "devs_i8080.h"
#include "serial_handler.h"

#define USART_BASE 0x00

namespace debugger {
namespace i8085 {

struct DevsI8085 final : i8080::DevsI8080 {
    DevsI8085();
    ~DevsI8085();

    void reset() override;
    void setIdle(bool idle) override;

    Device *parseDevice(const char *name) const override;
    void enableDevice(Device *dev) override;
    void printDevices() const override;

    void sioLoop() const {
#if defined(ENABLE_SERIAL_HANDLER)
        _sio->loop();
#endif
    }

private:
#if defined(ENABLE_SERIAL_HANDLER)
    SerialHandler *_sio;
#endif
};

}  // namespace i8085
}  // namespace debugger
#endif /* __DEVS_I8085H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
