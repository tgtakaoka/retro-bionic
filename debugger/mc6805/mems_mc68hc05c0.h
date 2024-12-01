#ifndef __MEMS_MC68HC05C0_H__
#define __MEMS_MC68HC05C0_H__

#include "mems_mc6805.h"

namespace debugger {
namespace mc68hc05c0 {

using mc6805::MemsMc6805;
using mc6805::RegsMc6805;

struct MemsMc68HC05C0 final : MemsMc6805 {
    MemsMc68HC05C0(RegsMc6805 &regs) : MemsMc6805(regs, 16, 0x240) {}
};

extern struct MemsMc68HC05C0 Memory;

}  // namespace mc68hc05c0
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
