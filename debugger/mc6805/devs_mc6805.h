#ifndef __DEVS_MC6805_H__
#define __DEVS_MC6805_H__

#include "devs.h"
#include "mc6850.h"

namespace debugger {
namespace mc6805 {

struct DevsMc6805 : Devs {
    DevsMc6805(uint16_t acia_base);
    ~DevsMc6805();

    void begin() override;
    void reset() override;
    bool loopDevices() override;
    // Inline for the bus cycles: Mc6850 is final.
    bool isSelected(uint32_t addr) const override {
        return _acia->isSelected(addr);
    }
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

    Device *parseDevice(const char *name) const override;
    void enableDevice(Device *dev) override;
    void printDevices() const override;

private:
    Mc6850 *const _acia;
};

}  // namespace mc6805
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
