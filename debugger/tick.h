#ifndef __DEBUGGER_TICK_H__
#define __DEBUGGER_TICK_H__

#include "device.h"

namespace debugger {

/**
 * A paravirtual tick timer on four byte-wide registers, for an OS's
 * clock: a periodic interrupt, 60 Hz after a reset, raised by name.
 *
 *   +0 STATUS   read: bit 7 set while a tick waits, and reading it
 *               acknowledges the tick; bit 0 set while running
 *      CONTROL  write: bit 0 runs the timer, and acknowledges a tick
 *   +1 INTR     the interrupt name a tick raises; 0 raises none
 *   +2 RATE     ticks per second, 1-255
 *
 * A tick that comes while one waits is lost.
 */
struct Tick final : Device {
    static constexpr uint8_t RATE_DEFAULT = 60;

    const char *name() const override { return "TICK"; }
    const char *description() const override { return "tick timer"; }
    void print() const override;

    void reset() override;
    bool loop() override;
    bool isSelected(uint32_t addr) const override {
        return _enabled && (addr & ~3) == _base_addr;
    }
    void setBaseAddr(uint32_t addr) override { _base_addr = addr & ~3; }
    uint32_t baseAddr() const override { return _base_addr; }
    uint16_t read(uint32_t addr) override;
    uint16_t peek(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) override;
    // Whether a tick waits with interrupt |name|.
    bool requesting(uint8_t name) const { return _pending && _intr == name; }

private:
    uint32_t _base_addr = 0;
    uint8_t _intr = 0;
    uint8_t _rate = RATE_DEFAULT;
    bool _run = false;
    bool _pending = false;
    uint32_t _due = 0;  // micros() of the next tick

    uint32_t period() const { return 1000000UL / _rate; }
    void acknowledge();
};

}  // namespace debugger
#endif /* __DEBUGGER_TICK_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
