#ifndef __MEMS_Z280_H__
#define __MEMS_Z280_H__

#include "mems.h"

namespace debugger {
namespace z280 {

struct MemsZ280 : ExtMemory {
    MemsZ280();

    uint32_t maxAddr() const override { return UINT32_C(0xFFFFFFFF); }
};

}  // namespace z280
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
