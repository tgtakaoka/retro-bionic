#ifndef __INST_MC6809_H__
#define __INST_MC6809_H__

#include "match_walker.h"
#include "mems.h"
#include "regs_mc6809.h"

namespace debugger {
namespace mc6809 {

using mc6809::SoftwareType;

// A CPU of the MC6809 family as MatchWalker sees it, its code in |mems|;
// its tables and interrupts come from its subclass.
struct InstMc6809 : MatchWalker::Arch {
    explicit InstMc6809(const MatchMemory *mems)
        : Arch(MatchWalker::Traits{.bigEndian = true,
                  .maxStart = 40,
                  .lastIsNextFetch = true}),
          _mems(mems) {}
    virtual ~InstMc6809() {}

    virtual void setSoftwareType(SoftwareType type) = 0;

    static constexpr uint8_t NOP = 0x12;
    static constexpr uint8_t BRA = 0x20;
    static constexpr uint8_t BRA_HERE = 0xFE;
    static constexpr uint8_t RTI = 0x3B;
    static constexpr uint8_t SWI = 0x3F;

    virtual uint16_t vec_swi() const { return 0xFFFA; }
    static constexpr uint16_t VEC_RESET = 0xFFFE;

    // Keeps the board's bus alive while a walk goes, if set.
    void setIdle(void (*idle)(void *), void *context) {
        _idle = idle;
        _context = context;
    }

    MatchWalker::Kind cycleKind(const SignalsImpl *s) const override;
    bool isVectorTable(uint32_t addr) const override {
        return addr >= vectorBase();
    }
    bool isDummy(const SignalsImpl *s) const override {
        return s->addr == 0xFFFF;
    }
    const char *interruptSequence() const override { return intrSequence(); }
    // CWAI's wait: dummy reads, then the vector, one more, and the
    // handler's fetch
    const char *resumeSequence() const override { return "{x}VrxP"; }
    void idle() const override {
        if (_idle)
            _idle(_context);
    }
    void markCycle(SignalsImpl *s, MatchWalker::Role role,
            uint_fast8_t span) const override;

protected:
    const MatchMemory *_mems;

    virtual const char *intrSequence() const { return nullptr; }
    // FIRQ, SWI2, SWI3 and the HD6309 trap sit below FFF8.
    virtual uint16_t vectorBase() const { return 0xFFF0; }

private:
    void (*_idle)(void *) = nullptr;
    void *_context = nullptr;
};

}  // namespace mc6809
}  // namespace debugger
#endif
// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
