#ifndef __INST_MC6800_H__
#define __INST_MC6800_H__

#include "match_walker.h"
#include "mems.h"
#include "signals_mc6800.h"

namespace debugger {
namespace mc6800 {

// A CPU of the MC6800 family as MatchWalker sees it, its code in |mems|;
// each one's tables and interrupt come from its subclass.
struct ArchMc6800 : MatchWalker::Arch {
    explicit ArchMc6800(const MatchMemory *mems)
        : Arch(MatchWalker::Traits{.bigEndian = true,
                  .maxStart = 13,
                  .lastIsNextFetch = true}),
          _mems(mems) {}
    virtual ~ArchMc6800() {}

    static constexpr uint8_t NOP = 0x01;
    static constexpr uint8_t BRA = 0x20;
    static constexpr uint8_t BRA_HERE = 0xFE;
    static constexpr uint8_t RTI = 0x3B;
    static constexpr uint8_t SWI = 0x3F;

    virtual uint16_t vec_swi() const { return 0xFFFA; }
    virtual uint16_t vec_nmi() const { return 0xFFFC; }
    static constexpr uint16_t VEC_RESET = 0xFFFE;

    // Keeps the board's bus alive while a walk goes, if set.
    void setIdle(void (*idle)(void *), void *context) {
        _idle = idle;
        _context = context;
    }

    MatchWalker::Kind cycleKind(const SignalsImpl *s) const override;
    bool decode(uint32_t pc, MatchWalker::Decoded &inst) const override;
    bool isVectorTable(uint32_t addr) const override {
        return addr >= vectorBase();
    }
    bool isDummy(const SignalsImpl *s) const override {
        return s->addr == 0xFFFF;
    }
    const char *interruptSequence() const override { return intrSequence(); }
    // WAI's wait: cycles of no address, or a read at one address over and
    // over, then the vector and the handler's fetch
    const char *resumeSequence() const override { return "{-}VrP@{R0}VrP"; }
    void idle() const override {
        if (_idle)
            _idle(_context);
    }
    void markCycle(SignalsImpl *s, MatchWalker::Role role,
            uint_fast8_t span) const override;

protected:
    const MatchMemory *_mems;

    virtual const char *instSequence(uint8_t) const { return ""; }
    virtual const char *intrSequence() const { return nullptr; }
    virtual uint16_t vectorBase() const { return 0xFFF8; }

private:
    void (*_idle)(void *) = nullptr;
    void *_context = nullptr;
};

}  // namespace mc6800
}  // namespace debugger
#endif
// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
