#ifndef __INST_HD6309_H__
#define __INST_HD6309_H__

#include "inst_mc6809.h"

namespace debugger {
namespace hd6309 {

using mc6809::InstMc6809;
using mc6809::SoftwareType;

struct InstHd6309 final : InstMc6809 {
    InstHd6309(const DmaMemory *mems) : InstMc6809(mems) {}

    bool match(const mc6800::Signals *begin, const mc6800::Signals *end,
            const mc6800::Signals *prefetch) override;
    bool matchInterrupt(
            const mc6800::Signals *begin, const mc6800::Signals *end) override;
    void setSoftwareType(SoftwareType type) override { _type = type; }

private:
    SoftwareType _type;
    bool _native6309;
    bool _matchingNative6309;

    struct StrBuffer;
    const char *assembleSequence(
            uint16_t fetch, const char *seq, StrBuffer &sequence) const;
    static void appendStackSequence(
            uint8_t post, uint8_t mask, char c, StrBuffer &sequence);
    static const char *copySequence(const char *seq, StrBuffer &buffer);
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
