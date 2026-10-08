#ifndef __INST_MB8861_H__
#define __INST_MB8861_H__

#include "inst_mc6800.h"

namespace debugger {
namespace mb8861 {

using mc6800::ArchMc6800;

struct InstMb8861 final : ArchMc6800 {
    InstMb8861(const MatchMemory *mems) : ArchMc6800(mems) {}

protected:
    const char *instSequence(uint8_t inst) const override;
    const char *intrSequence() const override;
};

}  // namespace mb8861
}  // namespace debugger
#endif
// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
