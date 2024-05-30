#ifndef __MEMS_NSC800_H__
#define __MEMS_NSC800_H__

#include "mems_z80.h"

namespace debugger {
namespace nsc800 {

using z80::MemsZ80;
using z80::RegsZ80;

struct MemsNsc800 final : MemsZ80 {
    MemsNsc800(RegsZ80 &regs) : MemsZ80(regs) {}
};

extern struct MemsNsc800 Memory;

}  // namespace nsc800
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
