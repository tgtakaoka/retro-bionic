#ifndef __DEVS_MC68HC11_H__
#define __DEVS_MC68HC11_H__

#include "devs.h"
#include "mc6850.h"
#include "mc68hc11_sci_handler.h"

#define ACIA_BASE 0xDF00

namespace debugger {
namespace mc68hc11 {

struct Mc68hc11Init;

struct DevsMc68hc11 final : Devs {
    DevsMc68hc11(Mc68hc11Init &init);
    ~DevsMc68hc11();

    void begin() override;
    void reset() override;
    bool loopDevices() override;
    void setIdle(bool idle) override;
    // Inline for the bus cycles: the devices are final.
    bool isSelected(uint32_t addr) const override {
#if defined(ENABLE_SERIAL_HANDLER)
        return _acia->isSelected(addr) || _sci->isSelected(addr);
#else
        return _acia->isSelected(addr);
#endif
    }
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

    Device *parseDevice(const char *name) const override;
    void enableDevice(Device *dev) override;
    void printDevices() const override;

    Mc68hc11Init &init() const { return _init; }

private:
    Mc68hc11Init &_init;
    Mc6850 *const _acia;
#if defined(ENABLE_SERIAL_HANDLER)
    Mc68hc11SciHandler *const _sci;
#endif
};

}  // namespace mc68hc11
}  // namespace debugger
#endif /* __DEVS_MC68HC11_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
