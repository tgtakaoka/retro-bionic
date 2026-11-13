#ifndef __MEMS_MC6802_H__
#define __MEMS_MC6802_H__

#include "mems_mc6800.h"
#include "regs_mc6802.h"

namespace debugger {
namespace mc6802 {

using mc6800::MemsMc6800Base;

struct MemsMc6802 final : MemsMc6800Base {
    MemsMc6802(RegsMc6802 *regs, DevsMc6800 *devs)
        : MemsMc6800Base(regs), _devs(devs) {}

    uint16_t read(uint32_t addr) const override {
        return _devs->isSelected(addr) ? _devs->read(addr) : read_byte(addr);
    }
    void write(uint32_t addr, uint16_t data) const override {
        if (_devs->isSelected(addr)) {
            _devs->write(addr, data);
        } else {
            write_byte(addr, data);
        }
    }

    uint16_t get_data(uint32_t addr) const override;
    void put_data(uint32_t addr, uint16_t data) const override;

    void set_internal_ram(bool enabled) { _internal_ram = enabled; }
    // On-chip RAM at 0000-007F, enabled by RE.
    bool is_internal(uint16_t addr) const {
        return _internal_ram && addr < 0x80;
    }

private:
    DevsMc6800 *const _devs;
    bool _internal_ram;
};

}  // namespace mc6802
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
