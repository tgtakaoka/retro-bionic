#ifndef __DEVS_MC146805E2_H__
#define __DEVS_MC146805E2_H__

#include "devs.h"

#define ACIA_BASE 0x17F8

namespace debugger {
namespace mc146805e2 {

struct DevsMc146805E2 final : Devs {
    DevsMc146805E2();
    ~DevsMc146805E2();

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
    Device *_acia;
};

}  // namespace mc146805e2
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
