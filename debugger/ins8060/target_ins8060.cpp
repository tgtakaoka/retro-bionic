#include "target.h"

#include "devs_ins8060.h"
#include "mems_ins8060.h"
#include "pins_ins8060.h"
#include "regs_ins8060.h"

namespace debugger {
namespace ins8060 {

struct Target TargetIns8060 {
    "INS8060", Pins, Regs, Memory, Devs
};

}  // namespace ins8060
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
