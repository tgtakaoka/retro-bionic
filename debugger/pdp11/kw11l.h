#ifndef __KW11L_H__
#define __KW11L_H__

#include "device.h"

namespace debugger {
namespace pdp11 {

/**
 * KW11-L line time clock: LKS, one word. MONITOR comes 60 times a second,
 * and with the interrupt enable set each one requests an interrupt, which
 * the CPU's acknowledge drops.
 */
struct Kw11l final : Device {
    // The interrupt's name, its vector.
    static constexpr uint8_t VECTOR = 0100;

    const char *name() const override;
    const char *description() const override;
    void print() const override;
    void reset() override;
    bool loop() override;
    void enable(bool enabled) override;
    bool isSelected(uint32_t addr) const override {
        return _enabled && addr == _base_addr;
    }
    void setBaseAddr(uint32_t addr) override { _base_addr = addr & ~1; }
    uint32_t baseAddr() const override { return _base_addr; }
    uint16_t read(uint32_t addr) override { return peek(addr); }
    uint16_t peek(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) override;

    // The CPU took the interrupt.
    void acknowledge();

private:
    uint32_t _base_addr = 0;
    uint32_t _tick = 0;  // when the last one came, in CPU cycles
    bool _monitor = false;
    bool _ie = false;
    bool _request = false;

    static constexpr uint16_t MONITOR_bm = 0200;
    static constexpr uint16_t IE_bm = 0100;
};

}  // namespace pdp11
}  // namespace debugger
#endif /* __KW11L_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
