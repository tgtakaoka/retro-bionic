#ifndef __MEMS_Z80_H__
#define __MEMS_Z80_H__

#include "mems.h"

namespace debugger {
namespace z80 {

struct RegsZ80;

struct MemsZ80 final : DmaMemory {
    MemsZ80();

    uint32_t maxAddr() const override { return UINT16_MAX; }
};

}  // namespace z80
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
