#include "target.h"

#include "devs_mc6801.h"
#include "mems_mc6801.h"
#include "pins_mc6801.h"
#include "regs_mc6801.h"

namespace debugger {
namespace mc6801 {

struct Target TargetMc6801 {
    "MC6801", Pins, Regs, Memory, Devices
};

}  // namespace mc6801
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
