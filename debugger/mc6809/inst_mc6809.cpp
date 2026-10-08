#include "inst_mc6809.h"
#include "signals_mc6809.h"

namespace debugger {
namespace mc6809 {

MatchWalker::Kind InstMc6809::cycleKind(const SignalsImpl *impl) const {
    const auto s = static_cast<const Signals *>(impl);
    if (!s->valid())
        return MatchWalker::K_NONE;
    return s->read() ? MatchWalker::K_READ : MatchWalker::K_WRITE;
}

// A fetch marked with the cycles its instruction took.
void InstMc6809::markCycle(
        SignalsImpl *impl, MatchWalker::Role role, uint_fast8_t span) const {
    const auto s = static_cast<Signals *>(impl);
    if (role == MatchWalker::R_FETCH) {
        s->markFetch(span);
    } else {
        s->clearFetch();
    }
}

}  // namespace mc6809
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
