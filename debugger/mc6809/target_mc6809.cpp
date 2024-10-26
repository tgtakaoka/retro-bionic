#include "target.h"

#include "devs_mc6809.h"
#include "inst_hd6309.h"
#include "mems_mc6809.h"
#include "pins_mc6809.h"
#include "regs_mc6809.h"

namespace debugger {
namespace mc6809 {

using hd6309::InstHd6309;

extern struct PinsMc6809 Pins;

struct RegsMc6809 Regs {
    Pins
};

struct MemsMc6809 Memory {
    Regs
};

struct InstHd6309 Inst {
    Memory
};

struct PinsMc6809 Pins {
    Regs, Inst, Memory
};

const struct Target TargetMc6809 {
    "MC6809", Pins, Regs, Memory, Devs
};

}  // namespace mc6809
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
