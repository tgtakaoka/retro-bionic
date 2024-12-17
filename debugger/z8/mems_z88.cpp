#include "mems_z88.h"
#include "regs_z88.h"

namespace debugger {
namespace z88 {

MemsZ88::MemsZ88(RegsZ88 *regs, Devs *devs) : MemsZ8(regs, devs) {}

}  // namespace z88
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
