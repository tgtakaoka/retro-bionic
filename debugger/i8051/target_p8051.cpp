#include "target.h"

#include "devs_i8051.h"
#include "inst_i8051.h"
#include "mems_i8051.h"
#include "pins_i8051.h"
#include "regs_i8051.h"

namespace debugger {
namespace i8051 {

struct Target TargetI8051 {
    "P8051", Pins, Regs, ProgMemory, Devs
};

}  // namespace i8051
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
