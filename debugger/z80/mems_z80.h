#ifndef __MEMS_Z80_H__
#define __MEMS_Z80_H__

#include "mems.h"

namespace debugger {
namespace z80 {

struct RegsZ80;

struct MemsZ80 final : DmaMemory {
    MemsZ80(RegsZ80 *regs);

    uint32_t maxAddr() const override { return UINT16_MAX; }
    uint16_t get(uint32_t addr, const char *space = nullptr) const override;
    void put(uint32_t addr, uint16_t data,
            const char *space = nullptr) const override;

private:
    RegsZ80 *const _regs;
};

}  // namespace z80
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
