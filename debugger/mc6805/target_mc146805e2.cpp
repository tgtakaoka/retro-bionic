#include "target.h"

#include "devs_mc6805.h"
#include "mems_mc146805e2.h"
#include "pins_mc146805e2.h"
#include "regs_mc146805e2.h"

namespace debugger {
namespace mc146805e2 {

struct MemsMc146805E2 Memory{Regs};
struct RegsMc146805E2 Regs{Pins, Memory};

const struct Target TargetMc146805E2{
        "MC146805E2",
        Pins,
        Regs,
        Memory,
        mc6805::Devices,
};

}  // namespace mc146805e2
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
