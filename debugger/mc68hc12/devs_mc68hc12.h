#ifndef __DEVS_MC68HC12_H__
#define __DEVS_MC68HC12_H__

#include "devs.h"
#include "serial_handler.h"

#define ACIA_BASE 0xDF00

namespace debugger {
namespace mc68hc12 {

struct Mc68hc12Init;

struct DevsMc68hc12 final : Devs {
    DevsMc68hc12(Mc68hc12Init &init);
    ~DevsMc68hc12();

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

private:
    Mc68hc12Init &_init;
    Device *_acia;
#if defined(ENABLE_SERIAL_HANDLER)
    SerialHandler *_sci;
#endif
};

}  // namespace mc68hc12
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
