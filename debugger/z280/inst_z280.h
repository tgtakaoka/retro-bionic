#ifndef __INST_Z280_H__
#define __INST_Z280_H__

#include <stdint.h>

#ifndef ARDUINO
#include <vector>
#endif

#include "match_memory.h"
#include "match_walker.h"

namespace debugger {
namespace z280 {

struct Signals;

// The Z280 as MatchWalker sees it, its code in |mems|: a queue of 4 bytes,
// stalls that fetch again, and an MMU between the PC and the bus.
struct InstZ280 final : MatchWalker::Arch {
    using Memory = MatchMemory;

    explicit InstZ280(const Memory *mems)
        : Arch(MatchWalker::Traits{.addressMask = 0xFFFFFF,
                  .queue = 4,
                  .refetch = 1,
                  .cutStart = 5}),
          _mems(mems) {}

    static constexpr uint8_t NOP = 0x00;
    static constexpr uint8_t HALT = 0x76;
    static constexpr uint8_t RETN_PREFIX = 0xED;
    static constexpr uint8_t RETN = 0x45;
    static constexpr uint8_t RETIL = 0x55;  // ED 55: pops MSR and PC
    // A Z-BUS fetch is a word, so RETN is injected as one.
    static constexpr uint16_t RETN_WORD = RETN_PREFIX << 8 | RETN;

    static constexpr uint8_t RET = 0xC9;
    static constexpr uint8_t JP = 0xC3;
    // A program hands control back to the monitor by writing RST 38H
    // into the restart vector and restarting to it. Shared convention
    // with the Z80 targets, so the samples work unchanged.
    static constexpr uint8_t RST38 = 0xFF;

    static constexpr uint16_t ORG_RESET = 0x0000;
    static constexpr uint16_t ORG_RST38 = 0x0038;
    // #NMI pushes PC and vectors here in interrupt modes 0, 1 and 2.
    static constexpr uint16_t ORG_NMI = 0x0066;

    // The walk of the ring [begin, end) that leaves the CPU at |pc|:
    // where it starts, |end| if there is none. Marks its cycles: fetch()
    // on each instruction's first fetch, isOperand() on data, isByte() on
    // the other fetches.
    static Signals *findFetch(Signals *begin, const Signals *end,
            const Memory &mems, uint32_t pc);

    MatchWalker::Kind cycleKind(const SignalsImpl *s) const override;
    bool decode(uint32_t pc, MatchWalker::Decoded &inst) const override;
#ifndef ARDUINO
    // The sequences it walks with, for a host test to check: its
    // instructions', its interrupts'.
    static std::vector<const char *> sequences();
    static std::vector<const char *> interrupts();
#endif
    uint_fast8_t dataBytes(const SignalsImpl *s) const override;
    uint8_t dataByte(const SignalsImpl *s, uint_fast8_t k) const override;
    bool sameAddress(uint32_t bus, uint32_t addr) const override;
    bool pushesPc(
            const SignalsImpl *s, uint32_t next, uint32_t last) const override;
    const char *interruptSequence() const override;
    void markCycle(SignalsImpl *s, MatchWalker::Role role,
            uint_fast8_t span) const override;

private:
    const Memory *_mems;
};

}  // namespace z280
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
