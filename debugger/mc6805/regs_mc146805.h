#ifndef __REGS_MC146805_H__
#define __REGS_MC146805_H__

#include "regs_mc6805.h"

namespace debugger {
namespace mc146805 {

struct RegsMc146805 final : mc6805::RegsMc6805 {
    RegsMc146805(mc6805::PinsMc6805 *pins) : RegsMc6805("MC146805", pins) {}
};

}  // namespace mc146805
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
