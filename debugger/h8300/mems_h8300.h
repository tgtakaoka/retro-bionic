#ifndef __MEMS_H8300_H__
#define __MEMS_H8300_H__

#include "mems.h"

namespace debugger {
namespace h8300 {

// A byte memory, big-endian: a word's even byte is its high one.
struct MemsH8300 final : DmaMemory {
    MemsH8300();

    uint32_t maxAddr() const override { return UINT16_MAX; }
};

}  // namespace h8300
}  // namespace debugger
#endif /* __MEMS_H8300_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
