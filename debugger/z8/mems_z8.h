#ifndef __MEMS_Z8_H__
#define __MEMS_Z8_H__

#include "devs_z8.h"
#include "mems.h"

namespace debugger {
namespace z8 {

struct MemsZ8 : DmaMemory {
    uint32_t maxAddr() const override { return UINT16_MAX; }
    // Final and inline for the bus cycles; no part overrides the byte
    // accessors.
    uint16_t read(uint32_t addr) const final {
        return _devs->isSelected(addr) ? _devs->read(addr)
                                       : DmaMemory::read_byte(addr);
    }
    void write(uint32_t addr, uint16_t data) const final {
        if (_devs->isSelected(addr)) {
            _devs->write(addr, data);
        } else {
            DmaMemory::write_byte(addr, data);
        }
    }

protected:
    MemsZ8(DevsZ8 *devs);
    DevsZ8 *const _devs;
};

}  // namespace z8
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
