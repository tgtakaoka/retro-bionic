#ifndef __MEMS_MC6802_H__
#define __MEMS_MC6802_H__

#include "mems_mc6800.h"
#include "regs_mc6802.h"

namespace debugger {
namespace mc6802 {

using mc6800::MemsMc6800;

struct MemsMc6802 final : MemsMc6800 {
    MemsMc6802(RegsMc6802 *regs, Devs *devs) : MemsMc6800(regs, devs) {}

    uint16_t get(uint32_t addr, const char *space = nullptr) const override;
    void put(uint32_t addr, uint16_t data,
            const char *space = nullptr) const override;

    void set_internal_ram(bool enabled) { _internal_ram = enabled; }
    bool is_internal(uint16_t addr) const {
        return _internal_ram && addr < 0x80;
    }

private:
    bool _internal_ram;
};

}  // namespace mc6802
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
