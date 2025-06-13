#ifndef __MEMS_SCN2650_H__
#define __MEMS_SCN2650_H__

#include "mems.h"

namespace debugger {
namespace scn2650 {

struct MemsScn2650 final : DmaMemory {
    MemsScn2650();

    uint32_t maxAddr() const override { return 0x7FFF; }
};

}  // namespace scn2650
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
