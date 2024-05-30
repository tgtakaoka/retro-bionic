#include "target.h"

#include "devs_z80.h"
#include "mems_nsc800.h"
#include "pins_nsc800.h"
#include "regs_nsc800.h"

namespace debugger {
namespace nsc800 {

using z80::Devs;

struct RegsNsc800 Regs {
    Pins,
};

struct MemsNsc800 Memory {
    Regs,
};

struct Target TargetNsc800 {
    "NSC800", Pins, Regs, Memory, Devs
};

}  // namespace nsc800
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
