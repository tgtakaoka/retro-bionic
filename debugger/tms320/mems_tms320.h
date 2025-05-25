#ifndef __MEMS_TMS320_H__
#define __MEMS_TMS320_H__

#include "mems.h"

namespace debugger {
namespace tms320 {

struct MemsTms320 : DmaMemory {
    MemsTms320();

    bool wordAccess() const override { return true; }

    uint16_t read(uint32_t addr) const override { return read_word(addr); }
    void write(uint32_t addr, uint16_t data) const override {
        write_word(addr, data);
    }
};

}  // namespace tms320
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
