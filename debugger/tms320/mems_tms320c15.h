#ifndef __MEMS_TMS320C15_H__
#define __MEMS_TMS320C15_H__

#include "mems_tms320.h"
#include "regs_tms3201x.h"

namespace debugger {
namespace tms320c15 {

using tms3201x::RegsTms3201X;

struct MemsTms320C15 final : tms320::MemsTms320 {
    MemsTms320C15(RegsTms3201X *regs) : MemsTms320(), _regs(regs) {}

    uint32_t maxAddr() const override { return UINT16_C(0xFFF); }
    uint32_t maxData() const override { return UINT8_MAX; }
    uint16_t get_data(uint32_t addr) const override {
        return _regs->read_data(addr);
    }
    void put_data(uint32_t addr, uint16_t data) const override {
        _regs->write_data(addr, data);
    }

private:
    RegsTms3201X *const _regs;
};

}  // namespace tms320c15
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
