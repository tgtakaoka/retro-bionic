#ifndef __DEVS_MC68000_H__
#define __DEVS_MC68000_H__

#include "devs.h"

namespace debugger {
namespace mc68000 {

// The MC6850 ACIA at FFFFF0H, reached as absolute short $FFF0.W on every
// board; a board with fewer address lines sees it at its masked address.
// Every interrupt is acknowledged with a vector number, since #VPA is tied
// high: the ACIA's is 64, the first user vector the samples use, unless
// `I ACIA addr rx tx` sets another.
struct DevsMc68000 final : Devs {
    static constexpr uint32_t ACIA_BASE = 0xFFFFF0;
    static constexpr uint8_t ACIA_VECTOR = 0x40;

    DevsMc68000(uint32_t addrMask);
    ~DevsMc68000();

    void begin() override;
    void reset() override;
    bool loopDevices() override;
    bool isSelected(uint32_t addr) const override;
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;
    uint16_t vector() const override;

    Device *parseDevice(const char *name) const override;
    void enableDevice(Device *dev) override;
    void printDevices() const override;

private:
    Device *_acia;
};

}  // namespace mc68000
}  // namespace debugger
#endif /* __DEVS_MC68000_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
