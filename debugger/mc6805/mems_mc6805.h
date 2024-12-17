#ifndef __MEMS_MC6805_H__
#define __MEMS_MC6805_H__

#include "devs.h"
#include "mems.h"

namespace debugger {
namespace mc6805 {

struct RegsMc6805;

struct MemsMc6805 : DmaMemory {
    MemsMc6805(RegsMc6805 *regs, Devs *devs, uint8_t pc_bits);

    uint32_t maxAddr() const override { return _max_addr; }
    uint16_t vecReset() const { return _max_addr - 1; }
    uint16_t vecSwi() const { return _max_addr - 3; }
    virtual bool is_internal(uint16_t addr) const = 0;

    uint16_t get(uint32_t addr, const char *space = nullptr) const override;
    void put(uint32_t addr, uint16_t data,
            const char *space = nullptr) const override;

protected:
    RegsMc6805 *_regs;
    Devs *_devs;
    const uint8_t _pc_bits;
    const uint16_t _max_addr;
};

}  // namespace mc6805
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
