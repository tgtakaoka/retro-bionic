#ifndef __DEBUGGER_DEVS_H__
#define __DEBUGGER_DEVS_H__

#include <stdint.h>
#include "config_debugger.h"

namespace debugger {

struct Device;

struct Devs {
    virtual ~Devs() {}

    virtual void begin() = 0;
    virtual void reset() = 0;
    virtual void loop() = 0;
    virtual void setIdle(bool idle) {}
    virtual bool isSelected(uint32_t addr) const { return false; }
    virtual uint16_t read(uint32_t addr) const { return 0; }
    virtual void write(uint32_t addr, uint16_t data) const {}
    virtual uint16_t vector() const { return 0; }

    virtual Device *parseDevice(const char *word) const { return nullDevice(); }
    virtual void enableDevice(Device *dev) {}
    virtual void printDevices() const {}

    static Device *nullDevice();

protected:
    void printDevice(const Device *dev) const;
};

}  // namespace debugger

#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
