#include "target.h"

#include "devs_f3850.h"
#include "mems_f3850.h"
#include "pins_f3850.h"
#include "regs_f3850.h"

namespace debugger {
namespace f3850 {

struct Target TargetF3850 {
    "F3850", Pins, Regs, Memory, Devs
};

}  // namespace f3850
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
