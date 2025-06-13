#ifndef __MEMS_MC68HC11_H__
#define __MEMS_MC68HC11_H__

#include "mems_mc6800.h"
#include "regs_mc68hc11.h"

namespace debugger {
namespace mc68hc11 {

struct Mc68hc11Init;

struct MemsMc68hc11 final : mc6800::MemsMc6800 {
    MemsMc68hc11(RegsMc68hc11 *regs, Devs *devs, Mc68hc11Init &init)
        : MemsMc6800(regs, devs), _init(init) {}

    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

    uint16_t get_data(uint32_t addr) const override;
    void put_data(uint32_t addr, uint16_t data) const override;

private:
    Mc68hc11Init &_init;

    RegsMc68hc11 *regs() const { return static_cast<RegsMc68hc11 *>(_regs); }
};

}  // namespace mc68hc11
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
