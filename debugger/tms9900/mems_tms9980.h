#ifndef __MEMS_TMS9980_H__
#define __MEMS_TMS9980_H__

#include "mems_tms9900.h"

namespace debugger {
namespace tms9980 {

// The bus is byte wide (read/write), but the debugger keeps 16-bit
// words, as on the TMS9900.
struct MemsTms9980 : tms9900::MemsTms9900 {
    MemsTms9980(Devs *devs);

    uint32_t maxAddr() const override { return UINT16_C(0x3FFF); }

    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

    // The word holding |addr|, as on the TMS9900.
    uint16_t get_prog(uint32_t addr) const override {
        return read16(addr & ~1);
    }
    void put_prog(uint32_t addr, uint16_t data) const override {
        write16(addr & ~1, data);
    }
};

}  // namespace tms9980
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
