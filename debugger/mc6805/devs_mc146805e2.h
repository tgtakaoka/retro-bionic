#ifndef __DEVS_MC146805E2_H__
#define __DEVS_MC146805E2_H__

#include "devs.h"

namespace debugger {
namespace mc146805e2 {

struct DevsMc146805E2 final : Devs {
    DevsMc146805E2(uint16_t acia) : _acia(acia) {}

    void begin() override;
    void reset() override;
    void loop() override;
    bool isSelected(uint32_t addr) const override;
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

    Device &parseDevice(const char *name) const override;
    void enableDevice(Device &dev) override;
    void printDevices() const override;

private:
    const uint16_t _acia;
};

extern struct DevsMc146805E2 Devices;

}  // namespace mc146805e2
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
