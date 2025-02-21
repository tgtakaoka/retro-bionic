#ifndef __MEMS_I8080_H__
#define __MEMS_I8080_H__

#include "mems.h"

namespace debugger {
namespace i8080 {

struct RegsI8080;

struct MemsI8080 final : DmaMemory {
    MemsI8080(RegsI8080 *regs);

    uint32_t maxAddr() const override { return UINT16_MAX; }
    uint16_t get(uint32_t addr, const char *space = nullptr) const override;
    void put(uint32_t addr, uint16_t data,
            const char *space = nullptr) const override;

private:
    RegsI8080 *const _regs;
};

}  // namespace i8080
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
