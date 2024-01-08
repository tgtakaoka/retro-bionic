#include "target.h"

#include "devs_cdp1802.h"
#include "mems_cdp1802.h"
#include "pins_cdp1802.h"
#include "regs_cdp1802.h"

namespace debugger {
namespace cdp1802 {

struct Target TargetCdp1802 {
    "CDP1802", Pins, Regs, Memory, Devs
};

}  // namespace cdp1802
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
