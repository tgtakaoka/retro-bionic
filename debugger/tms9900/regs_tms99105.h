#ifndef __REGS_TMS99105_H__
#define __REGS_TMS99105_H__

#include "regs_tms9900.h"

namespace debugger {
namespace tms99105 {

struct PinsTms99105;

struct RegsTms99105 final : tms9900::RegsTms9900 {
    RegsTms99105(PinsTms99105 *pins, Mems *mems);

    const char *cpu() const override;

    void save() override;
    void restore() override;

    void breakPoint() override;

private:
    const char *regs_line() const override;
};

}  // namespace tms99105
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
