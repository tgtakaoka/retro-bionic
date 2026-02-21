#ifndef __DEVS_MC68HC05C0_H__
#define __DEVS_MC68HC05C0_H__

#include "devs_mc6805.h"

namespace debugger {
namespace mc68hc05c0 {

#if defined(ENABLE_SERIAL_HANDLER)
struct DevsMc68HC05C0 final : mc6805::DevsMc6805 {
    DevsMc68HC05C0(uint16_t acia_base);
    ~DevsMc68HC05C0();

    void reset() override;
    void loop() override;
    void setIdle(bool idle) override;
    bool isSelected(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

    Device *parseDevice(const char *name) const override;
    void enableDevice(Device *dev) override;
    void printDevices() const override;

private:
    SerialHandler *_sci;
};
#else
struct DevsMc68HC05C0 final : mc6805::DevsMc6805 {
    DevsMc68HC05C0(uint16_t acia_base) : DevsMc6805(acia_base) {}
};
#endif

}  // namespace mc68hc05c0
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
