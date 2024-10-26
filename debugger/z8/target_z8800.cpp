#include "target.h"

#include "devs_z88.h"
#include "mems_z88.h"
#include "pins_z88.h"
#include "regs_z88.h"

namespace debugger {
namespace z88 {

const struct Target TargetZ88 {
    "Z88C00", Pins, Regs, Memory, Devs
};

}  // namespace z88
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
