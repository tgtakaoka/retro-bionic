#ifndef __DEVS_SCN2650_H__
#define __DEVS_SCN2650_H__

#include "devs.h"

#define USART_BASE 0x00

namespace debugger {
namespace scn2650 {

struct DevsScn2650 final : Devs {
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

extern struct DevsScn2650 Devs;

}  // namespace scn2650
}  // namespace debugger
#endif /* __DEVS_F3850H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
