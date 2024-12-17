#ifndef __MEMS_SCN2650_H__
#define __MEMS_SCN2650_H__

#include "mems.h"

namespace debugger {
namespace scn2650 {

struct RegsScn2650;

struct MemsScn2650 final : DmaMemory {
    MemsScn2650(RegsScn2650 *regs);

    uint32_t maxAddr() const override { return 0x7FFF; }
    uint16_t get(uint32_t addr, const char *space = nullptr) const override;
    void put(uint32_t addr, uint16_t data,
            const char *space = nullptr) const override;

private:
    RegsScn2650 *_regs;
};

extern struct MemsScn2650 Memory;

}  // namespace scn2650
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
