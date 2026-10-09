#ifndef __INST_I8096_H__
#define __INST_I8096_H__

#include "match_memory.h"
#include "match_walker.h"
#include "signals_i8096.h"

namespace debugger {
namespace i8096 {

// An instruction's sequence, from the tables.
struct InstI8096 final {
    bool set(uint16_t pc, const MatchMemory *mems);
    uint_fast8_t opc() const { return _opc; }
    const char *sequence() const { return _seq; }
    uint_fast8_t instLength() const;

    static constexpr uint16_t ORG_RESET = 0x2080;
    static constexpr uint16_t VEC_EXTINT = 0x200E;
    static constexpr uint16_t VEC_TRAP = 0x2010;

    static constexpr uint8_t TRAP = 0xF7;
#define SJMP(disp) (0x20 | ((disp >> 8) & 7)), (disp & 0xFF)

private:
    uint_fast8_t _opc;
    const char *_seq;

    static bool indexAddressing(uint_fast8_t opc);
};

// The 8096 as MatchWalker sees it, its code in |mems|: a queue of 4 bytes,
// fetched a word at a time on a 16-bit bus.
struct ArchI8096 final : MatchWalker::Arch {
    explicit ArchI8096(const MatchMemory *mems)
        : Arch(MatchWalker::Traits{.maxStart = 14,
                  .queue = 4,
                  .cutStart = 7,
                  .refetchWord = true}),
          _mems(mems) {}

    // Keeps the board's bus alive while a walk goes, if set.
    void setIdle(void (*idle)(void *), void *context) {
        _idle = idle;
        _context = context;
    }

    MatchWalker::Kind cycleKind(const SignalsImpl *s) const override;
    bool decode(uint32_t pc, MatchWalker::Decoded &inst) const override;
    bool isVectorTable(uint32_t addr) const override;
    uint_fast8_t fetchBytes(const SignalsImpl *s) const override {
        return static_cast<const SignalsI8096 *>(s)->bytes();
    }
    uint_fast8_t dataBytes(const SignalsImpl *s) const override {
        return static_cast<const SignalsI8096 *>(s)->bytes();
    }
    uint8_t dataByte(const SignalsImpl *impl, uint_fast8_t i) const override {
        const auto s = static_cast<const SignalsI8096 *>(impl);
        return s->byteAt(s->addr + i);
    }
    const char *interruptSequence() const override;
    void idle() const override {
        if (_idle)
            _idle(_context);
    }
    void markCycle(SignalsImpl *s, MatchWalker::Role role,
            uint_fast8_t span) const override;
    void markStart(SignalsImpl *s, uint32_t pc) const override;

private:
    const MatchMemory *_mems;
    void (*_idle)(void *) = nullptr;
    void *_context = nullptr;
};

}  // namespace i8096
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
