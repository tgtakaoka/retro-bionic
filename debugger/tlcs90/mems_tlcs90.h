#ifndef __MEMS_TLCS90_H__
#define __MEMS_TLCS90_H__

#include "devs.h"
#include "mems.h"

namespace debugger {
namespace tlcs90 {

struct RegsTlcs90;

struct MemsTlcs90 final : DmaMemory {
    MemsTlcs90(RegsTlcs90 *regs, Devs *devs);

    uint32_t maxAddr() const override { return UINT16_MAX; }
    static bool is_internal(uint16_t addr) { return addr >= 0xFEC0; }

    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;
    uint16_t get_data(uint32_t addr) const override;
    void put_data(uint32_t addr, uint16_t data) const override;

private:
    RegsTlcs90 *const _regs;
    Devs *const _devs;
};

}  // namespace tlcs90
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
