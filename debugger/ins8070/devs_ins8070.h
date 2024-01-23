#ifndef __DEVS_INS8070_H__
#define __DEVS_INS8070_H__

#include "devs.h"

#define ACIA_BASE 0xDF00

namespace debugger {
namespace ins8070 {

struct DevsIns8070 final : Devs {
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

extern struct DevsIns8070 Devs;

}  // namespace ins8070
}  // namespace debugger
#endif /* __DEVS_INS8070H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
