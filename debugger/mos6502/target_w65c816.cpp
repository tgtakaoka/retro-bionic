#include "target_w65c816.h"
#include "devs_mos6502.h"
#include "mems_w65c816.h"
#include "pins_mos6502.h"
#include "regs_mos6502.h"

namespace debugger {
namespace w65c816 {

using mos6502::Devices;
using mos6502::Pins;
using mos6502::Registers;

const struct Target TargetW65c816 {
    "W65816", Pins, Registers, Memory, Devices
};

}  // namespace w65c816
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
