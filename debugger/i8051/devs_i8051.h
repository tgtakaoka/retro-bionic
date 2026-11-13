#ifndef __DEVS_I8051_H__
#define __DEVS_I8051_H__

#include "devs.h"
#include "i8051_uart_handler.h"
#include "i8251.h"

#define USART_BASE 0xFFF0

namespace debugger {
namespace i8051 {

struct DevsI8051 final : Devs {
    DevsI8051();
    ~DevsI8051();

    void begin() override;
    void reset() override;
    bool loopDevices() override;
    void setIdle(bool idle) override;
    // Inline for the bus cycles: I8251 is final.
    bool isSelected(uint32_t addr) const override {
        return _usart->isSelected(addr);
    }
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
    I8251 *const _usart;
#if defined(ENABLE_SERIAL_HANDLER)
    I8051UartHandler *const _uart;
#endif
};

}  // namespace i8051
}  // namespace debugger
#endif /* __DEVS_I8051_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
