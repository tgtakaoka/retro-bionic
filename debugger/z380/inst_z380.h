#ifndef __INST_Z380_H__
#define __INST_Z380_H__

#include <stdint.h>

#ifndef ARDUINO
#include <vector>
#endif

#include "match_memory.h"
#include "match_walker.h"

namespace debugger {
namespace z380 {

struct Signals;

// The Z380 as MatchWalker sees it, in Extended (|xm|) and Long Word (|lw|)
// mode. It fetches code as a stream of aligned words, up to 5 bytes ahead
// of what it runs: each instruction's bytes are fetched, then its data
// cycles and the fetch at its target, if it transfers control, come in any
// order while the stream goes on. An interrupt, NMI or trap is the PC
// pushed and a vector fetched, in either order, between two instructions.
struct InstZ380 final : MatchWalker::Arch {
    using Memory = MatchMemory;

    InstZ380(const Memory *mems, bool xm, bool lw)
        : Arch(MatchWalker::Traits{.addressMask = UINT32_MAX,
                  .queue = 8,
                  .refetch = 8,
                  .cutStart = 6,
                  .leadUnproven = true}),
          _mems(mems),
          _xm(xm),
          _lw(lw) {}

    static constexpr uint8_t NOP = 0x00;
    static constexpr uint8_t HALT = 0x76;
    static constexpr uint8_t RETN_PREFIX = 0xED;
    static constexpr uint8_t RETN = 0x45;
    static constexpr uint8_t JP = 0xC3;
    // A program hands control back to the monitor by writing RST 38H
    // into the restart vector and restarting to it. Shared convention
    // with the Z80 targets, so the samples work unchanged.
    static constexpr uint8_t RST38 = 0xFF;

    static constexpr uint16_t ORG_RESET = 0x0000;
    static constexpr uint16_t ORG_RST38 = 0x0038;
    // #NMI pushes PC and vectors here, with no acknowledge transaction.
    static constexpr uint16_t ORG_NMI = 0x0066;

    // The walk of the ring [begin, end) that leaves the CPU at |pc|, in
    // MatchWalker::shared(). Marks its cycles: fetch() on each
    // instruction's first fetch, isOperand() on data, isByte() on the
    // other fetches.
    static bool walk(Signals *begin, const Signals *end, const Memory &mems,
            uint32_t pc, bool xm, bool lw);

    MatchWalker::Kind cycleKind(const SignalsImpl *s) const override;
    bool decode(uint32_t pc, MatchWalker::Decoded &inst) const override;
#ifndef ARDUINO
    // The sequences it walks with, for a host test to check: its
    // instructions', its interrupts'.
    static std::vector<const char *> sequences();
    static std::vector<const char *> interrupts();
#endif
    uint_fast8_t fetchBytes(const SignalsImpl *s) const override;
    uint_fast8_t dataBytes(const SignalsImpl *s) const override;
    uint8_t dataByte(const SignalsImpl *s, uint_fast8_t k) const override;
    bool entersInterrupt(
            const SignalsImpl *s, const SignalsImpl *ack) const override;
    bool pushesPc(
            const SignalsImpl *s, uint32_t next, uint32_t last) const override;
    const char *interruptSequence() const override;
    void markCycle(SignalsImpl *s, MatchWalker::Role role,
            uint_fast8_t span) const override;

private:
    const Memory *_mems;
    const bool _xm;
    const bool _lw;
};

}  // namespace z380
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
