#include "target.h"

#include "devs_i8085.h"
#include "mems_i8085.h"
#include "pins_i8085.h"
#include "regs_i8085.h"

namespace debugger {
namespace i8085 {

const struct Target TargetI8085 {
    "P8085", Pins, Regs, Memory, Devs
};

}  // namespace i8085
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
