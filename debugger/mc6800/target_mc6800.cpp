#include "target.h"

#include "devs_mc6800.h"
#include "inst_mb8861.h"
#include "mems_mc6800.h"
#include "pins_mc6800.h"
#include "regs_mc6800.h"

namespace debugger {
namespace mc6800 {

using mb8861::InstMb8861;
extern struct PinsMc6800 Pins;

struct RegsMc6800 Regs {
    Pins
};

struct MemsMc6800 Memory {
    Regs
};

struct InstMb8861 Inst {
    Memory,
};

struct PinsMc6800 Pins {
    Regs, Inst, Memory, Devices
};

const struct Target TargetMc6800 {
    "MC6800", Pins, Regs, Memory, Devices
};

}  // namespace mc6800
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
