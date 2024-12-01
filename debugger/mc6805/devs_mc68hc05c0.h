#ifndef __DEVS_MC68HC05C0_H__
#define __DEVS_MC68HC05C0_H__

#include "devs_mc6805.h"

#define ACIA_BASE 0xFFE0

namespace debugger {
namespace mc68hc05c0 {

using mc6805::DevsMc6805;

struct DevsMc68HC05C0 final : DevsMc6805 {
    DevsMc68HC05C0() : DevsMc6805(ACIA_BASE) {}
};

extern struct DevsMc68HC05C0 Devices;

}  // namespace mc68hc05c0
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
