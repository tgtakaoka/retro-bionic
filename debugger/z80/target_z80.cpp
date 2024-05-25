#include "target.h"

#include "devs_z80.h"
#include "mems_z80.h"
#include "pins_z80.h"
#include "regs_z80.h"

namespace debugger {
namespace z80 {

struct Target TargetZ80 {
    "Z80", Pins, Regs, Memory, Devs
};

}  // namespace z80
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
