#ifndef __DEVS_MC6801_H__
#define __DEVS_MC6801_H__

#include "devs.h"

#define ACIA_BASE 0xDF00

namespace debugger {
namespace mc6801 {

struct Mc6801SciHandler;

struct DevsMc6801 final : Devs {
    DevsMc6801(Mc6801SciHandler *sci) : _sci(sci) {}

    void begin() override;
    void reset() override;
    void loop() override;
    void setIdle(bool idle) override;
    bool isSelected(uint32_t addr) const override;
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

    Device &parseDevice(const char *name) const override;
    void enableDevice(Device &dev) override;
    void printDevices() const override;

private:
    Mc6801SciHandler *const _sci;
};

extern struct DevsMc6801 Devices;

}  // namespace mc6801
}  // namespace debugger
#endif /* __DEVS_MC6801H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
