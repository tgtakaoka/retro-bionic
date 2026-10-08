#ifndef __INST_TLCS90_H__
#define __INST_TLCS90_H__

#include "match_walker.h"
#include "mems.h"
#include "signals_tlcs90.h"

namespace debugger {
namespace tlcs90 {

// The TLCS-90 as MatchWalker sees it, its code in |mems|.
struct InstTlcs90 final : MatchWalker::Arch {
    explicit InstTlcs90(const MatchMemory *mems)
        : Arch(MatchWalker::Traits{.maxStart = 14, .lastIsNextFetch = true}),
          _mems(mems) {}

    static bool valid(uint16_t addr, Mems *mems);

    static constexpr uint8_t HALT = 0x01;
    static constexpr uint8_t SWI = 0xFF;
    static constexpr uint16_t ORG_SWI = 0x0010;
    static constexpr uint16_t ORG_NMI = 0x0018;
    // The on-chip I/O registers, which the bus never shows.
    static constexpr uint16_t INTERNAL_IO = 0xFFC0;
    static constexpr uint16_t INTERNAL_IO_END = 0xFFEF;

    // Keeps the board's bus alive while a walk goes, if set.
    void setIdle(void (*idle)(void *), void *context) {
        _idle = idle;
        _context = context;
    }

    MatchWalker::Kind cycleKind(const SignalsImpl *s) const override;
    bool decode(uint32_t pc, MatchWalker::Decoded &inst) const override;
    bool isVectorTable(uint32_t addr) const override;
    const char *interruptSequence() const override;
    void idle() const override {
        if (_idle)
            _idle(_context);
    }
    void markCycle(SignalsImpl *s, MatchWalker::Role role,
            uint_fast8_t span) const override;

private:
    const MatchMemory *_mems;
    void (*_idle)(void *) = nullptr;
    void *_context = nullptr;
};
}  // namespace tlcs90
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
