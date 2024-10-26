#include "target.h"

#include "devs_ins8070.h"
#include "mems_ins8070.h"
#include "pins_ins8070.h"
#include "regs_ins8070.h"

namespace debugger {
namespace ins8070 {

const struct Target TargetIns8070 {
    "INS8070", Pins, Regs, Memory, Devs
};

}  // namespace ins8070
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
