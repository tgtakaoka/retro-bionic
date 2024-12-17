#include "mems_z86.h"
#include "regs_z86.h"

namespace debugger {
namespace z86 {

MemsZ86::MemsZ86(RegsZ86 *regs, Devs *devs) : MemsZ8(regs, devs) {};

}  // namespace z86
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
