#ifndef __DEVS_MC68HC11_H__
#define __DEVS_MC68HC11_H__

#include "devs.h"

#define ACIA_BASE 0xDF00

namespace debugger {
namespace mc68hc11 {

struct Mc68hc11Init;

struct DevsMc68hc11 final : Devs {
    DevsMc68hc11(Mc68hc11Init &init);;
    ~DevsMc68hc11();

    void begin() override;
    void reset() override;
    void loop() override;
    void setIdle(bool idle) override;
    bool isSelected(uint32_t addr) const override;
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

    Device *parseDevice(const char *name) const override;
    void enableDevice(Device *dev) override;
    void printDevices() const override;

    Mc68hc11Init &init() const { return _init; }

private:
    Mc68hc11Init &_init;
    Device *_acia;
#if defined(ENABLE_SERIAL_HANDLER)
    SerialHandler *_sci;
#endif
};

}  // namespace mc68hc11
}  // namespace debugger
#endif /* __DEVS_MC68HC11DH__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
