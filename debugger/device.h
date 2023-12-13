#ifndef __DEBUGGER_DEVICE_H__
#define __DEBUGGER_DEVICE_H__

#include <stdint.h>

namespace debugger {

struct Device {
    Device() : _enabled(false) {}

    virtual const char *name() const = 0;
    virtual const char *description() const = 0;
    virtual void print() const {}

    virtual void reset() = 0;
    virtual void loop() = 0;
    void enable(bool enabled) { _enabled = enabled; }
    bool isEnabled() const { return _enabled; }

    virtual bool isSelected(uint32_t addr) const { return addr == baseAddr(); }
    virtual uint16_t read(uint32_t addr) { return 0; }
    virtual void write(uint32_t addr, uint16_t data) {}
    virtual void setBaseAddr(uint32_t addr) {}
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
