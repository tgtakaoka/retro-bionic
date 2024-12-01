#include "target.h"

#include "devs_mc6805.h"
#include "mems_mc68hc05c0.h"
#include "pins_mc68hc05c0.h"
#include "regs_mc68hc05c0.h"

namespace debugger {
namespace mc68hc05c0 {

struct MemsMc68HC05C0 Memory{Regs};
struct RegsMc68HC05C0 Regs{Pins, Memory};

const struct Target TargetMc68hc05c0{
        "MC68HC05C0",
        Pins,
        Regs,
        Memory,
        mc6805::Devices,
};

}  // namespace mc68hc05c0
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
