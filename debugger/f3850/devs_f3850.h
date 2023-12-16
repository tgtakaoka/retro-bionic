#ifndef __DEVS_F3850_H__
#define __DEVS_F3850_H__

#include "devs.h"

#define USART_BASE 0xF0

namespace debugger {
namespace f3850 {

struct DevsF3850 final : Devs {
    void begin() override;
    void reset() override;
    void loop() override;
    bool isSelected(uint32_t addr) const override;
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;
    uint16_t vector() const override;

    Device &parseDevice(const char *name) const override;
    void enableDevice(Device &dev) override;
    void printDevices() const override;
};

extern struct DevsF3850 Devs;

}  // namespace f3850
}  // namespace debugger
#endif /* __DEVS_F3850H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
