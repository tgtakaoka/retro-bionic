#ifndef __DEVS_MOS6502_H__
#define __DEVS_MOS6502_H__

#include "devs.h"
#include "mc6850.h"

#define ACIA_BASE 0xDF00

namespace debugger {
namespace mos6502 {

struct DevsMos6502 final : Devs {
    DevsMos6502();
    ~DevsMos6502();

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

}  // namespace mos6502
}  // namespace debugger
#endif /* __DEVS_MOS6502_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
