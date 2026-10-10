#include "pipe_mc68hc16.h"

namespace debugger {
namespace mc68hc16 {

namespace {
constexpr int_fast16_t NONE = -1;
// Phase 1: START and FETCH are active low on IPIPE0 and IPIPE1.
constexpr bool start(uint8_t phase1) {
    return (phase1 & 1) == 0;
}
constexpr bool fetch(uint8_t phase1) {
    return (phase1 & 2) == 0;
}
// Phase 2: 00 INVALID, 01 ADVANCE, 10 EXCEPTION, 11 NULL.
constexpr uint8_t INVALID = 0;
constexpr uint8_t ADVANCE = 1;
constexpr uint8_t EXCEPTION = 2;
}  // namespace

void PipeMc68hc16::track(
        const Cycle *cycles, uint_fast16_t n, int16_t *starts) {
    int_fast16_t stageA = NONE, stageB = NONE;
    for (uint_fast16_t i = 0; i < n; ++i) {
        const auto &c = cycles[i];
        starts[i] = NOT_START;
        // An acknowledge, or a cycle whose state is flagged invalid,
        // carries no pipeline state to act on.
        if (c.iack || c.phase2 == INVALID)
            continue;
        if (start(c.phase1))
            starts[i] = stageB == NONE ? 0 : i - stageB;
        if (c.phase2 == ADVANCE) {
            stageB = stageA;
            stageA = NONE;
        } else if (c.phase2 == EXCEPTION) {
            stageA = stageB = NONE;
        }
        if (fetch(c.phase1) && c.program)
            stageA = i;
    }
}

}  // namespace mc68hc16
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
