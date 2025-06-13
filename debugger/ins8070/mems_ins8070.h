#ifndef __MEMS_INS8070_H__
#define __MEMS_INS8070_H__

#include "devs.h"
#include "mems.h"

namespace debugger {
namespace ins8070 {

struct RegsIns8070;

struct MemsIns8070 final : DmaMemory {
    MemsIns8070(RegsIns8070 *regs, Devs *devs);

    uint32_t maxAddr() const override { return UINT16_MAX; }
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;
    uint16_t get_data(uint32_t addr) const override;
    void put_data(uint32_t addr, uint16_t data) const override;

    static bool is_internal(uint16_t addr) { return addr >= 0xFFC0; }

private:
    RegsIns8070 *const _regs;
    Devs *const _devs;
};

}  // namespace ins8070
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
