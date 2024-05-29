#include "target.h"

#include "devs_mc6800.h"
#include "inst_mb8861.h"
#include "mems_mc6802.h"
#include "pins_mc6802.h"
#include "regs_mc6802.h"

namespace debugger {
namespace mc6802 {

using mb8861::InstMb8861;
using mc6800::Devices;
extern struct PinsMc6802 Pins;

struct RegsMc6802 Regs {
    Pins
};

struct MemsMc6802 Memory {
    Regs
};

struct InstMb8861 Inst {
    Memory,
};

struct PinsMc6802 Pins {
    Regs, Inst, Memory, Devices
};

struct Target TargetMc6802 {
    "MC6802", Pins, Regs, Memory, Devices
};

}  // namespace mc6802
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
