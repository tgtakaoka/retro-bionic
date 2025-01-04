#ifndef __DEVS_PDP8_H__
#define __DEVS_PDP8_H__

#include "devs.h"

#define ACIA_BASE 06000

namespace debugger {
namespace pdp8 {

struct DevsPdp8 final : Devs {
    DevsPdp8();
    ~DevsPdp8();

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

}  // namespace pdp8
}  // namespace debugger
#endif /* __DEVS_PDP8H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
