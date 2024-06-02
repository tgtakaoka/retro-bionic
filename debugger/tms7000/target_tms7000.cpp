#include "target.h"

#include "devs_tms7000.h"
#include "mems_tms7000.h"
#include "pins_tms7000.h"
#include "regs_tms7000.h"

namespace debugger {
namespace tms7000 {

struct Target TargetTms7000 {
    "TMS7000", Pins, Regs, Memory, Devs
};

}  // namespace tms7000
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
