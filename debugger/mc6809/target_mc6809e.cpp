#include "target.h"

#include "devs_mc6809.h"
#include "inst_hd6309.h"
#include "mems_mc6809.h"
#include "pins_mc6809e.h"
#include "regs_mc6809e.h"

namespace debugger {
namespace mc6809e {

using hd6309::InstHd6309;
using mc6809::Devs;
using mc6809::MemsMc6809;

extern struct PinsMc6809E Pins;

struct RegsMc6809E Regs {
    &Pins
};

struct MemsMc6809 Memory {
    &Regs
};

struct InstHd6309 Inst {
    &Memory
};

struct PinsMc6809E Pins {
    &Regs, &Inst, &Memory
};

struct Target TargetMc6809 {
    "MC6809E", Pins, Regs, Memory, Devs
};

}  // namespace mc6809e
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
