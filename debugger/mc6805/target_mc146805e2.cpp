#include "target.h"

#include "devs_mc146805e2.h"
#include "mems_mc146805e2.h"
#include "pins_mc146805e2.h"
#include "regs_mc146805e2.h"

namespace debugger {
namespace mc146805e2 {

struct RegsMc146805E2 Regs {
    &Pins, &Memory
};

struct Target TargetMc146805E2 {
    "MC146805E2", Pins, Regs, Memory, Devices
};

}  // namespace mc146805e2
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
