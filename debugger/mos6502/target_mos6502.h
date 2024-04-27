#ifndef __TARGET_MOS6502_H__
#define __TARGET_MOS6502_H__

#include "target.h"

namespace debugger {
namespace mos6502 {

struct TargetMos6502 final : Target {
    TargetMos6502(const char *id, debugger::Pins &pins, Regs &regs, Mems &mems,
            Devs &devs)
        : Target(id, pins, regs, mems, devs) {}

    void setMems(Mems &mems) { _mems = &mems; }
    Mems &memory() { return *_mems; }
};

extern struct TargetMos6502 Target6502;

}  // namespace mos6502
}  // namespace debugger

#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
