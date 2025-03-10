#ifndef __REGS_MC68HC05_H__
#define __REGS_MC68HC05_H__

#include "regs_mc6805.h"

namespace debugger {
namespace mc68hc05 {

struct RegsMc68HC05 final : mc6805::RegsMc6805 {
    RegsMc68HC05(mc6805::PinsMc6805 *pins);

    void reset() override;
};

}  // namespace mc68hc05
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
