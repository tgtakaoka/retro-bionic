#include "target.h"

#include "devs_i8048.h"
#include "inst_i8048.h"
#include "mems_i8048.h"
#include "pins_i8048.h"
#include "regs_i8048.h"

namespace debugger {
namespace p8039 {

using i8048::Devs;
using i8048::InstI8048;
using i8048::PinsI8048;
using i8048::ProgI8048;
using i8048::RegsI8048;

extern RegsI8048 Regs;
constexpr char CPU_NAME[] = "P8039";
const InstI8048 Inst{InstI8048::F_I8039};

ProgI8048 ProgMemory{Regs};
PinsI8048 Pins{Regs, ProgMemory, Inst};
RegsI8048 Regs{"i8039", CPU_NAME, Pins};

struct Target TargetI8039 {
    CPU_NAME, Pins, Regs, ProgMemory, Devs
};

}  // namespace p8039
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
