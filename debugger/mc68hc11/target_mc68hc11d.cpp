#include "target.h"

#include "devs_mc68hc11.h"
#include "inst_mc68hc11.h"
#include "mc68hc11_init.h"
#include "mc68hc11_sci_handler.h"
#include "mems_mc68hc11.h"
#include "pins_mc68hc11.h"
#include "regs_mc68hc11.h"

namespace debugger {
namespace mc68hc11d {

using mc68hc11::DevsMc68hc11;
using mc68hc11::InstMc68hc11;
using mc68hc11::Mc68hc11Init;
using mc68hc11::Mc68hc11SciHandler;
using mc68hc11::MemsMc68hc11;
using mc68hc11::PinsMc68hc11;
using mc68hc11::RegsMc68hc11;

extern struct PinsMc68hc11 Pins;
extern struct RegsMc68hc11 Regs;

struct Mc68hc11Init Init {
    0x00, 0x40, 192, 0x3F, &Regs
};

struct Mc68hc11SciHandler SciH {
    &Init
};

struct DevsMc68hc11 Devices {
    &Init, &SciH
};

struct MemsMc68hc11 Memory {
    &Regs, &Devices
};

struct RegsMc68hc11 Regs {
    &Pins, &Memory
};

struct InstMc68hc11 Inst {
    &Memory
};

struct PinsMc68hc11 Pins {
    &Regs, &Inst, &Memory, &Devices
};

struct Target TargetMc68hc11D {
    "MC68HC11D", Pins, Regs, Memory, Devices
};

}  // namespace mc68hc11d
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
