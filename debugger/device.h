#ifndef __DEBUGGER_DEVICE_H__
#define __DEBUGGER_DEVICE_H__

#include <Arduino.h>
#include <stdint.h>

namespace debugger {

// A device's slow service, such as a UART's console work, runs once in
// 10 us.
constexpr uint32_t SERVICE_CYCLES = F_CPU / 100000;

// An emulated peripheral, selected by address on the target's bus.
struct Device {
    Device() : _enabled(false) {}
    virtual ~Device() {}

    virtual const char *name() const = 0;
    virtual const char *description() const = 0;
    virtual void print() const {}

    virtual void reset() = 0;
    // Devs calls it once in SERVICE_CYCLES, or on every bus cycle for a
    // device that counts them. True when it ran its slow service.
    virtual bool loop() = 0;
    virtual void enable(bool enabled) { _enabled = enabled; }
    virtual bool isEnabled() const { return _enabled; }

    virtual bool isSelected(uint32_t addr) const { return addr == baseAddr(); }
    virtual uint16_t read(uint32_t addr) { return 0; }
    // What read() would return, without its side effects.
    virtual uint16_t peek(uint32_t addr) const { return 0; }
    virtual void write(uint32_t addr, uint16_t data) {}
    virtual void setBaseAddr(uint32_t addr) {}
    virtual void setVector(uint8_t rx, uint8_t tx) {}
    virtual uint32_t baseAddr() const { return 0; }
    virtual uint16_t vector() const { return 0; }

protected:
    bool _enabled;
};

}  // namespace debugger

#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
