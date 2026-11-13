#ifndef __MEMS_TMS7000_H__
#define __MEMS_TMS7000_H__

#include "devs_tms7000.h"
#include "mems.h"

namespace debugger {
namespace tms7000 {

struct RegsTms7000;

struct MemsTms7000 final : DmaMemory {
    MemsTms7000(RegsTms7000 *regs, DevsTms7000 *devs);

    uint32_t maxAddr() const override { return UINT16_MAX; }
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
    RegsTms7000 *const _regs;
    DevsTms7000 *const _devs;
};

}  // namespace tms7000
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
