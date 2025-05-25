#ifndef __MEMS_TMS320C15_H__
#define __MEMS_TMS320C15_H__

#include "mems_tms320.h"

namespace debugger {
namespace tms320c15 {

struct MemsTms320C15 final : tms320::MemsTms320 {
    MemsTms320C15() : MemsTms320() {}

    uint32_t maxAddr() const override { return UINT16_C(0xFFF); }
};

}  // namespace tms320c15
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
