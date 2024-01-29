#ifndef __INST_MC6801_H__
#define __INST_MC6801_H__

#include "mc6800/inst_mc6800.h"

namespace debugger {
namespace mc6801 {

using mc6800::InstMc6800;

struct InstMc6801 final : InstMc6800 {
    InstMc6801(const DmaMemory *mems) : InstMc6800(mems) {}

protected:
    const char *instSequence(uint8_t inst) const override;
    const char *intrSequence() const override;
    uint16_t vectorBase() const override { return 0xFFF0; }
};

extern struct InstMc6801 Inst;

}  // namespace mc6801
}  // namespace debugger
#endif
// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
