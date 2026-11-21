#ifndef __SWR_H__
#define __SWR_H__

#include "device.h"

namespace debugger {
namespace pdp11 {

/**
 * Console switch register, one word that only reads; a write, to the
 * display register behind it, is lost. Unix starts single-user on 173030,
 * the setting at power-up, multi-user on any other, and its kernel prints
 * on any but 0. `I SWR value` sets it: the address stays.
 */
struct Swr final : Device {
    static constexpr uint16_t ADDR = 0177570;

    const char *name() const override { return "SWR"; }
    const char *description() const override { return "Switches"; }
    void print() const override;
    void reset() override {}
    bool loop() override { return false; }
    bool isSelected(uint32_t addr) const override {
        return _enabled && addr == ADDR;
    }
    // The number `I` takes is the setting.
    void setBaseAddr(uint32_t value) override { _value = value; }
    uint32_t baseAddr() const override { return ADDR; }
    uint16_t read(uint32_t addr) override { return _value; }
    uint16_t peek(uint32_t addr) const override { return _value; }

private:
    uint16_t _value = 0173030;
};

}  // namespace pdp11
}  // namespace debugger
#endif /* __SWR_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
