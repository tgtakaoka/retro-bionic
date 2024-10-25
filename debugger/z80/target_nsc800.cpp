#include "target.h"

#include "devs_z80.h"
#include "mems_z80.h"
#include "pins_nsc800.h"
#include "regs_z80.h"

namespace debugger {
namespace nsc800 {

extern struct PinsNsc800 Pins;

const char CPU[] = "NSC800";

struct z80::RegsZ80 Regs {
    CPU, Pins
};

struct z80::MemsZ80 Memory {
    Regs
};

struct PinsNsc800 Pins {
    Regs, Memory
};

using z80::Devs;

const struct Target TargetNsc800 {
    CPU, Pins, Regs, Memory, Devs
};

}  // namespace nsc800
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
