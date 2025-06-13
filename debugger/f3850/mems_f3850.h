#ifndef __MEMS_F3850_H__
#define __MEMS_F3850_H__

#include "mems.h"

namespace debugger {
namespace f3850 {

struct RegsF3850;

struct MemsF3850 final : DmaMemory {
    MemsF3850(RegsF3850 *regs);

    uint32_t maxAddr() const override { return UINT16_MAX; }
    uint16_t get_data(uint32_t addr) const override;
    void put_data(uint32_t addr, uint16_t data) const override;

private:
    RegsF3850 *const _regs;
};

}  // namespace f3850
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
