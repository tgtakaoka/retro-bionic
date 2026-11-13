#ifndef __MEMS_MC6800_H__
#define __MEMS_MC6800_H__

#include "devs_mc6800.h"
#include "mems.h"

namespace debugger {
namespace mc6800 {

struct RegsMc6800;

// Each part's final Mems holds its own Devs type, so that the bus cycles
// call read() and write() directly.
struct MemsMc6800Base : DmaMemory {
    uint32_t maxAddr() const override { return UINT16_MAX; }

protected:
    MemsMc6800Base(RegsMc6800 *regs);

    RegsMc6800 *const _regs;
};

struct MemsMc6800 final : MemsMc6800Base {
    MemsMc6800(RegsMc6800 *regs, DevsMc6800 *devs)
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

private:
    DevsMc6800 *const _devs;
};

}  // namespace mc6800
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
