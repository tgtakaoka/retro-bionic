#ifndef __REGS_Z180_H__
#define __REGS_Z180_H__

#include "regs_z80.h"

namespace debugger {
namespace z180 {

struct RegsZ180 final : z80::RegsZ80 {
    RegsZ180(z80::PinsZ80Base *pins) : RegsZ80(pins) {}

    const char *cpu() const override { return "Z180"; }
};

}  // namespace z180
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
