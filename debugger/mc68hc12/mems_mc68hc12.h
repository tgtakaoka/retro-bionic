#ifndef __MEMS_MC68HC12_H__
#define __MEMS_MC68HC12_H__

#include "devs.h"
#include "mems.h"

namespace debugger {
namespace mc68hc12 {

struct Mc68hc12Init;
struct RegsMc68hc12;

struct MemsMc68hc12 final : DmaMemory {
    MemsMc68hc12(RegsMc68hc12 *regs, Devs *devs, Mc68hc12Init &init);

    uint32_t maxAddr() const override { return UINT16_MAX; }
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

    // Internal registers, RAM and EEPROM are reached through the CPU.
    uint16_t get_data(uint32_t addr) const override;
    void put_data(uint32_t addr, uint16_t data) const override;

private:
    RegsMc68hc12 *const _regs;
    Devs *const _devs;
    Mc68hc12Init &_init;
};

}  // namespace mc68hc12
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
