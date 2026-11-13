#ifndef __MEMS_MC6801_H__
#define __MEMS_MC6801_H__

#include "devs_mc6801.h"
#include "mems_mc6800.h"
#include "regs_mc6801.h"

namespace debugger {
namespace mc6801 {

using mc6800::MemsMc6800Base;

struct MemsMc6801 final : MemsMc6800Base {
    MemsMc6801(RegsMc6801 *regs, DevsMc6801 *devs)
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

private:
    DevsMc6801 *const _devs;

    static constexpr uint16_t RAMCR = 0x0014;
    static constexpr uint8_t RAME_bm = 0x40;
};

}  // namespace mc6801
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
