#ifndef __TIMER_I8086_H__
#define __TIMER_I8086_H__

#include "device.h"

namespace debugger {
namespace i8086 {

/**
 * A paravirtual interval timer on four byte-wide I/O ports: the PC's
 * 18.2 Hz tick, raised on INTR with its own type until acknowledged.
 *
 *   +0 CTRL  bit 0: run; read: bit 7 set while a tick waits
 *   +1 TYPE  the interrupt type it answers INTA with
 *   +2 EOI   write: acknowledge the tick
 *
 * A tick that arrives while one waits is lost. Enabled at power-up; I
 * TIMER toggles it.
 */
struct Timer final : Device {
    static constexpr uint32_t PERIOD_US = 54925;  // 1193182 / 65536 Hz

    const char *name() const override { return "TIMER"; }
    const char *description() const override { return "18.2 Hz tick"; }
    void print() const override;

    void reset() override;
    bool loop() override;
    bool isSelected(uint32_t addr) const override {
        return _enabled && (addr & ~3) == _base_addr;
    }
    void setBaseAddr(uint32_t addr) override { _base_addr = addr & ~3; }
    uint32_t baseAddr() const override { return _base_addr; }
    uint16_t read(uint32_t addr) override { return peek(addr); }
    uint16_t peek(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) override;
    uint16_t vector() const override { return _pending ? _type : 0; }

private:
    uint32_t _base_addr = 0;
    uint8_t _type = 8;
    bool _run = false;
    bool _pending = false;
    uint32_t _due = 0;  // micros() of the next tick

    void acknowledge();
};

}  // namespace i8086
}  // namespace debugger
#endif /* __TIMER_I8086_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
