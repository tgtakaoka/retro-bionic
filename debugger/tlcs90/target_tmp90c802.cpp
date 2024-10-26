#include "target.h"

#include "devs_tlcs90.h"
#include "mems_tlcs90.h"
#include "pins_tlcs90.h"
#include "regs_tlcs90.h"

namespace debugger {
namespace tlcs90 {

const struct Target TargetTmp90c802 {
    "TMP90C802", Pins, Regs, Memory, Devs
};

}  // namespace tlcs90
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
