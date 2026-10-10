#ifndef __PIPE_MC68HC16_H__
#define __PIPE_MC68HC16_H__

#include <stdint.h>

namespace debugger {
namespace mc68hc16 {

// The CPU16 instruction pipeline as IPIPE1:IPIPE0 track it (CPU16RM
// 10.1.3). FETCH puts the cycle's word in stage A, ADVANCE moves stage A
// to stage B, and START begins the instruction whose opcode is in stage
// B; an EXCEPTION flushes the pipeline. Evaluated START, then ADVANCE,
// then FETCH, as the manual orders them.
struct PipeMc68hc16 {
    struct Cycle {
        uint8_t phase1;  // IPIPE1:IPIPE0 as #AS opens the cycle
        uint8_t phase2;  // IPIPE1:IPIPE0 late in the cycle
        bool program;    // a read of program space, which FETCH loads
        bool iack;       // an interrupt acknowledge: no pipeline state
    };
    static constexpr int16_t NOT_START = -1;
    // For each of |n| cycles in bus order: NOT_START, or for a cycle in
    // which an instruction started, how many cycles back its opcode word
    // was fetched -- 0 when the fetch predates the first cycle.
    static void track(const Cycle *cycles, uint_fast16_t n, int16_t *starts);
};

}  // namespace mc68hc16
}  // namespace debugger
#endif /* __PIPE_MC68HC16_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
