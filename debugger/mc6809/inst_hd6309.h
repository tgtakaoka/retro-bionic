#ifndef __INST_HD6309_H__
#define __INST_HD6309_H__

#include "inst_mc6809.h"

namespace debugger {
namespace hd6309 {

using mc6809::SoftwareType;

// The MC6809 and the HD6309, as MatchWalker sees them.
struct InstHd6309 final : mc6809::InstMc6809 {
    explicit InstHd6309(const MatchMemory *mems) : InstMc6809(mems) {}

    void setSoftwareType(SoftwareType type) override { _type = type; }
    bool decode(uint32_t pc, MatchWalker::Decoded &inst) const override;

protected:
    const char *intrSequence() const override;

private:
    SoftwareType _type = mc6809::SW_MC6809;
};

}  // namespace hd6309
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
