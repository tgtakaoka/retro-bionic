#include "target_mos6502.h"
#include "devs_mos6502.h"
#include "mems_mos6502.h"
#include "pins_mos6502.h"
#include "regs_mos6502.h"

namespace debugger {
namespace mos6502 {

struct TargetMos6502 Target6502 {
    "MOS6502", Pins, Registers, Memory, Devices
};

}  // namespace mos6502
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
