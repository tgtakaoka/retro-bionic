#ifndef __MEMS_I8080_H__
#define __MEMS_I8080_H__

#include "mems.h"

namespace debugger {
namespace i8080 {

struct MemsI8080 final : DmaMemory {
    MemsI8080();

    uint32_t maxAddr() const override { return UINT16_MAX; }
};

}  // namespace i8080
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
