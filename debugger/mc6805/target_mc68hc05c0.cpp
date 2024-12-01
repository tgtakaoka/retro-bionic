#include "target.h"

#include "devs_mc68hc05c0.h"
#include "inst_mc68hc05.h"
#include "mems_mc68hc05c0.h"
#include "pins_mc68hc05c0.h"
#include "regs_mc68hc05c0.h"

namespace debugger {
namespace mc68hc05c0 {

struct DevsMc68HC05C0 Devices;
struct MemsMc68HC05C0 Memory{Regs, Devices};
struct RegsMc68HC05C0 Regs{Pins, Memory};
struct PinsMc68HC05C0 Pins{Regs, mc68hc05::Inst, Memory, Devices};

const struct Target TargetMc68hc05c0{
        "MC68HC05C0",
        Pins,
        Regs,
        Memory,
        Devices,
};

}  // namespace mc68hc05c0
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
