#ifndef __MEMS_TMS7000_H__
#define __MEMS_TMS7000_H__

#include "devs.h"
#include "mems.h"

namespace debugger {
namespace tms7000 {

struct RegsTms7000;

struct MemsTms7000 final : DmaMemory {
    MemsTms7000(RegsTms7000 *regs, Devs *devs);

    uint32_t maxAddr() const override { return UINT16_MAX; }
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;
    uint16_t get(uint32_t addr, const char *space = nullptr) const override;
    void put(uint32_t addr, uint16_t data,
            const char *space = nullptr) const override;

private:
    RegsTms7000 *_regs;
    Devs *_devs;
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
