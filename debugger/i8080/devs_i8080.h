#ifndef __DEVS_I8080_H__
#define __DEVS_I8080_H__

#include "devs.h"
#include "serial_handler.h"

#define USART_BASE 0x00

namespace debugger {
namespace i8080 {

struct DevsI8080 : Devs {
    DevsI8080();
    ~DevsI8080();

    void begin() override;
    void reset() override;
    void loop() override;
    bool isSelected(uint32_t addr) const override;
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;
    uint16_t vector() const override;

    Device *parseDevice(const char *name) const override;
    void enableDevice(Device *dev) override;
    void printDevices() const override;

protected:
    Device *_usart;
};

}  // namespace i8080
}  // namespace debugger
#endif /* __DEVS_I8080H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
