#ifndef __MEMS_Z88_H__
#define __MEMS_Z88_H__

#include "mems_z8.h"

namespace debugger {
namespace z88 {

struct RegsZ88;

struct MemsZ88 final : z8::MemsZ8 {
    MemsZ88(RegsZ88 *regs, Devs *devs);
};

}  // namespace z88
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
