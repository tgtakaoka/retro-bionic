#ifndef __MEMS_Z8000_H__
#define __MEMS_Z8000_H__

#include "mems.h"

namespace debugger {
namespace z8000 {

// A byte memory, big-endian: a word's even byte is its high one, on
// AD8-AD15. Program words are read and written whole.
struct MemsZ8000 : ExtMemory {
    MemsZ8000();

    uint32_t maxAddr() const override { return UINT16_MAX; }

    uint16_t read(uint32_t addr) const override { return read16(addr & ~1); }
    void write(uint32_t addr, uint16_t data) const override {
        write16(addr & ~1, data);
    }
};

}  // namespace z8000
}  // namespace debugger
#endif /* __MEMS_Z8000_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
