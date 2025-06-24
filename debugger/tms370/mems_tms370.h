#ifndef __MEMS_TMS370_H__
#define __MEMS_TMS370_H__

#include "devs.h"
#include "mems.h"

namespace debugger {
namespace tms370 {

struct RegsTms370;
struct PinsTms370;

struct MemsTms370 : DmaMemory {
    MemsTms370(RegsTms370 *regs, PinsTms370 *pins, Devs *devs);

    uint32_t maxAddr() const override { return UINT16_MAX; }
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;
    uint16_t get_data(uint32_t addr) const override;
    void put_data(uint32_t addr, uint16_t data) const override;
    virtual bool internal(uint32_t addr) const = 0;

private:
    RegsTms370 *const _regs;
    PinsTms370 *const _pins;
    Devs *const _devs;
};

}  // namespace tms370
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
