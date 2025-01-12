#ifndef __DEVS_TLCS90_H__
#define __DEVS_TLCS90_H__

#include "devs.h"
#include "serial_handler.h"

#define USART_BASE 0xFFF0

namespace debugger {
namespace tlcs90 {

struct DevsTlcs90 final : Devs {
    DevsTlcs90();
    ~DevsTlcs90();

    void begin() override;
    void reset() override;
    void loop() override;
    void setIdle(bool idle) override;
    bool isSelected(uint32_t addr) const override;
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

    Device *parseDevice(const char *name) const override;
    void enableDevice(Device *dev) override;
    void printDevices() const override;

    void uartLoop() const {
#if defined(ENABLE_SERIAL_HANDLER)
        _uart->loop();
#endif
    }

private:
    Device *_usart;
#if defined(ENABLE_SERIAL_HANDLER)
    SerialHandler *_uart;
#endif
};

}  // namespace tlcs90
}  // namespace debugger
#endif /* __DEVS_TLCS90H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
