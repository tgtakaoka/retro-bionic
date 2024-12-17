#ifndef __DEVS_Z8_H__
#define __DEVS_Z8_H__

#include "devs.h"
#include "serial_handler.h"

#define USART_BASE 0xFF00

namespace debugger {
namespace z8 {

struct DevsZ8 final : Devs {
    DevsZ8(SerialHandler *serial);
    ~DevsZ8();

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

private:
    Device *_usart;
    SerialHandler *_serial;
};

}  // namespace z8
}  // namespace debugger
#endif /* __DEVS_Z86H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
