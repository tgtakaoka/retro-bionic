#ifndef __REGS_NSC800_H__
#define __REGS_NSC800_H__

#include "pins_z80.h"
#include "regs_z80.h"

namespace debugger {
namespace nsc800 {

using z80::PinsZ80;
using z80::RegsZ80;

struct RegsNsc800 final : RegsZ80 {
    RegsNsc800(PinsZ80 &pins) : RegsZ80(pins) {}

    const char *cpuName() const override { return "NSC800"; }
};

extern struct RegsNsc800 Regs;

}  // namespace nsc800
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
