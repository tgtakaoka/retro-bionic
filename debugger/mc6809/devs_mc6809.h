#ifndef __DEVS_MC6809_H__
#define __DEVS_MC6809_H__

#include "devs.h"
#include "mc6850.h"
#include "ram_disk.h"
#include "tick.h"

namespace debugger {
namespace mc6809 {

// The I/O page DF00-DFFF: the MC6850 ACIA at DF00, the tick timer at DF08
// and the RAM disk at DF10. Each raises IRQ or FIRQ by the name a program
// gives it, 0F8H or 0F6H, on the one line they share.
struct DevsMc6809 final : Devs {
    static constexpr uint32_t ACIA_BASE = 0xDF00;
    static constexpr uint32_t TICK_BASE = 0xDF08;
    static constexpr uint32_t DISK_BASE = 0xDF10;

    DevsMc6809();
    ~DevsMc6809();

    void begin() override;
    void reset() override;
    bool loopDevices() override;
    void setIdle(bool idle) override;
    // Inline through the final devices: the bus loop asks every cycle.
    bool isSelected(uint32_t addr) const override {
        return _acia->isSelected(addr) || _tick->isSelected(addr) ||
               _disk->isSelected(addr);
    }
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;
    // Whether any device still requests interrupt |name|.
    bool requesting(uint8_t name) const {
        return _acia->vector() == name || _tick->requesting(name) ||
               _disk->requesting(name);
    }

    Device *parseDevice(const char *name) const override;
    void enableDevice(Device *dev) override;
    void printDevices() const override;
    Disk *disk() const override { return _disk; }

private:
    Mc6850 *const _acia;
    Tick *const _tick;
    RamDisk *const _disk;
};

}  // namespace mc6809
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
