#include "target.h"

#include "devs_i8048.h"
#include "inst_i8048.h"
#include "mems_i8048.h"
#include "pins_i8048.h"
#include "regs_i8048.h"

namespace debugger {
namespace p8039 {

using i8048::Devs;
using i8048::Pins;
using i8048::ProgMemory;
using i8048::Regs;
using i8048::RegsI8048;

struct Target TargetI8039 {
    RegsI8048::P8039, Pins, Regs, ProgMemory, Devs
};

}  // namespace p8039
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
