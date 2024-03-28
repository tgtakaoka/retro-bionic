#ifndef __DEVS_I8048_H__
#define __DEVS_I8048_H__

#include "devs.h"

#define USART_BASE 0xFC

namespace debugger {
namespace i8048 {

struct DevsI8048 final : Devs {
    void begin() override;
    void reset() override;
    void loop() override;
    bool isSelected(uint32_t addr) const override;
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

    Device &parseDevice(const char *name) const override;
    void enableDevice(Device &dev) override;
    void printDevices() const override;
};

extern struct DevsI8048 Devs;

}  // namespace i8048
}  // namespace debugger
#endif /* __DEVS_I8048H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
