#ifndef __INST_HD6301_H__
#define __INST_HD6301_H__

#include "mc6800/inst_mc6800.h"

namespace debugger {
namespace hd6301 {

using mc6800::InstMc6800;

struct InstHd6301 final : InstMc6800 {
    InstHd6301(Mems *mems) : InstMc6800(mems) {}

protected:
    const char *instSequence(uint8_t inst) const override;
    const char *intrSequence() const override;
    uint16_t vectorBase() const override { return 0xFFF0; }
};

}  // namespace hd6301
}  // namespace debugger
#endif
// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
