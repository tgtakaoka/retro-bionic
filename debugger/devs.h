#ifndef __DEBUGGER_DEVS_H__
#define __DEBUGGER_DEVS_H__

#include <stdint.h>
#include "config_debugger.h"
#include "device.h"

namespace debugger {

// The emulated devices on a target's bus.
struct Devs {
    virtual ~Devs() {}

    virtual void begin() = 0;
    virtual void reset() = 0;
    // Called from every bus cycle. True when a device ran its slow
    // service, or else once in SERVICE_CYCLES: the cycle on which the bus
    // loop takes the halt switch, so a run stops with no device enabled.
    bool loop() {
        const auto now = ARM_DWT_CYCCNT;
        if (loopDevices() || now - _serviced >= SERVICE_CYCLES) {
            _serviced = now;
            return true;
        }
        return false;
    }
    // Whether the CPU is halted: false for the length of each run.
    virtual void setIdle(bool idle) {}
    virtual bool isSelected(uint32_t addr) const { return false; }
    virtual uint16_t read(uint32_t addr) const { return 0; }
    virtual void write(uint32_t addr, uint16_t data) const {}
    virtual uint16_t vector() const { return 0; }

    virtual Device *parseDevice(const char *word) const { return nullDevice(); }
    virtual void enableDevice(Device *dev) {}
    virtual void printDevices() const {}

    static Device *nullDevice();

private:
    uint32_t _serviced = 0;

protected:
    virtual bool loopDevices() = 0;
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
