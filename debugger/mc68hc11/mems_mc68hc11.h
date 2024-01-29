#ifndef __MEMS_MC68HC11_H__
#define __MEMS_MC68HC11_H__

#include "mc6800/mems_mc6800.h"
#include "regs_mc68hc11.h"

namespace debugger {
namespace mc68hc11 {

using mc6800::MemsMc6800;

struct DevsMc68hc11;

struct MemsMc68hc11 final : MemsMc6800 {
    MemsMc68hc11(RegsMc68hc11 *regs, DevsMc68hc11 *devs)
        : MemsMc6800(regs), _devs(devs) {}

    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

    uint16_t get(uint32_t addr, const char *space = nullptr) const override;
    void put(uint32_t addr, uint16_t data,
            const char *space = nullptr) const override;

    bool is_internal(uint16_t addr) const;

private:
    DevsMc68hc11 *const _devs;
    RegsMc68hc11 &regs() const { return *static_cast<RegsMc68hc11 *>(_regs); }
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
