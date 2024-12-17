#include "target.h"

#include "devs_scn2650.h"
#include "mems_scn2650.h"
#include "pins_scn2650.h"
#include "regs_scn2650.h"

namespace debugger {
namespace scn2650 {

const struct Target TargetScn2650 {
    "SCN2650", Pins, Regs, Memory, Devs
};

}  // namespace scn2650
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
