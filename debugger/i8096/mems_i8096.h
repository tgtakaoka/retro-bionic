#ifndef __MEMS_I8096_H__
#define __MEMS_I8096_H__

#include "devs.h"
#include "mems.h"

namespace debugger {
namespace i8096 {

struct RegsI8096;

struct MemsI8096 : DmaMemory {
    MemsI8096(Devs *devs, RegsI8096 *regs);

    uint32_t maxAddr() const override { return UINT16_MAX; }

    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

    uint16_t get_prog(uint32_t addr) const override;
    void put_prog(uint32_t addr, uint16_t data) const override;

private:
    Devs *const _devs;
    RegsI8096 *const _regs;
};

}  // namespace i8096
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
