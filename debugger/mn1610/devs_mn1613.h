#ifndef __DEVS_MN1613_H__
#define __DEVS_MN1613_H__

#include "devs.h"

#define USART_BASE 0x0080

namespace debugger {
namespace mn1613 {

enum HardwareType : uint8_t;

struct DevsMn1613 : Devs {
    DevsMn1613();
    ~DevsMn1613();

    void begin() override;
    void reset() override;
    void loop() override;
    bool isSelected(uint32_t addr) const override;
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

    Device *parseDevice(const char *name) const override;
    void enableDevice(Device *dev) override;
    void printDevices() const override;

private:
    Device *_usart;
};

}  // namespace mn1613
}  // namespace debugger
#endif /* __DEVS_MN1613H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
