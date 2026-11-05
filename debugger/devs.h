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
    // Called from every bus cycle; services the devices once in
    // SERVICE_CYCLES and returns true then: the cycle on which the bus
    // loop takes the halt switch, so a run stops with no device enabled.
    bool loop() {
        if (_everyCycle)
            _everyCycle->loop();
        const auto now = ARM_DWT_CYCCNT;
        if (now - _serviced < SERVICE_CYCLES)
            return false;
        _serviced = now;
        loopDevices();
        return true;
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
    // A device that counts bus cycles, called on every one.
    Device *_everyCycle = nullptr;

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
