#ifndef __DEVS_MC146805E2_H__
#define __DEVS_MC146805E2_H__

#include "devs_mc6805.h"

// For ASSIST05
#define ACIA_BASE 0x17F8

namespace debugger {
namespace mc146805e2 {

using mc6805::DevsMc6805;

struct DevsMc146805E2 final : DevsMc6805 {
    DevsMc146805E2() : DevsMc6805(ACIA_BASE) {}
};

extern struct DevsMc146805E2 Devices;

}  // namespace mc146805e2
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
