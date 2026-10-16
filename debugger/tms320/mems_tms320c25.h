#ifndef __MEMS_TMS320C25_H__
#define __MEMS_TMS320C25_H__

#include "mems_tms320.h"
#include "regs_tms320c2x.h"

namespace debugger {
namespace tms320c25 {

using tms320c2x::RegsTms320C2X;

// The board answers #PS and #DS from one 64K-word memory; the on-chip data
// below 0400h (memory-mapped registers, B2, B0 and B1) is reached by
// injected instructions.
struct MemsTms320C25 final : tms320::MemsTms320 {
    MemsTms320C25(RegsTms320C2X *regs) : MemsTms320(), _regs(regs) {}

    uint32_t maxAddr() const override { return UINT16_MAX; }
    uint16_t get_data(uint32_t addr) const override {
        return addr < ON_CHIP ? _regs->read_data(addr) : read(addr);
    }
    void put_data(uint32_t addr, uint16_t data) const override {
        if (addr < ON_CHIP) {
            _regs->write_data(addr, data);
        } else {
            write(addr, data);
        }
    }

    static constexpr uint16_t ON_CHIP = 0x400;

private:
    RegsTms320C2X *const _regs;
};

}  // namespace tms320c25
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
