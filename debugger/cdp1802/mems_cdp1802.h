#ifndef __MEMS_CDP1802_H__
#define __MEMS_CDP1802_H__

#include "mems.h"

namespace debugger {
namespace cdp1802 {

struct MemsCdp1802 final : DmaMemory {
    MemsCdp1802();

    uint32_t maxAddr() const override { return UINT16_MAX; }
};

}  // namespace cdp1802
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
