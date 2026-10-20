#ifndef __INST_I8096_H__
#define __INST_I8096_H__

#include "match_memory.h"
#include "match_walker.h"
#include "signals_i8096.h"

namespace debugger {
namespace i8096 {

// The instruction sets, each the one before's and more.
enum CpuType : uint8_t {
    CPU_8096,
    CPU_80C196KB,
    CPU_80C196KC,
};

// An instruction's sequence, from the tables, as |cpu| has it.
struct InstI8096 final {
    bool set(uint16_t pc, const MatchMemory *mems, CpuType cpu);
    // An 80C196 pushes the PC before it reads a vector, the 8096 after.
    // ATTENTION: seen on the S80C196KB and KC; the later ones follow them
    // unverified.
    static bool pushesBeforeVector(CpuType cpu) { return cpu >= CPU_80C196KB; }
    uint_fast8_t opc() const { return _opc; }
    const char *sequence() const { return _seq; }
    uint_fast8_t instLength() const;

    static constexpr uint16_t ORG_RESET = 0x2080;
    static constexpr uint16_t VEC_EXTINT = 0x200E;
    static constexpr uint16_t VEC_TRAP = 0x2010;
    // Whether a read at |addr| is an interrupt vector's: one taken.
    static bool vector(uint16_t addr) {
        return addr >= 0x2000 && addr < 0x2040;
    }

    // Where an 8096's NMI calls, with no vector read.
    static constexpr uint16_t ORG_NMI = 0x0000;
    // The 80C196's NMI vector.
    static constexpr uint16_t VEC_NMI = 0x203E;

    static constexpr uint8_t TRAP = 0xF7;
    static constexpr uint8_t RST = 0xFF;
    static constexpr uint8_t BMOV = 0xC1;
    static constexpr uint8_t BMOVI = 0xCD;
#define SJMP(disp) (0x20 | ((disp >> 8) & 7)), (disp & 0xFF)

private:
    uint_fast8_t _opc;
    const char *_seq;

    static bool indexAddressing(uint_fast8_t opc);
};

// The 8096 as MatchWalker sees it, its code in |mems|: a queue of 4 bytes,
// fetched a word at a time on a 16-bit bus.
// The instruction bytes an i8096 holds past a running instruction: an
// 80C196's 4-byte queue (8XC196Kx user's manual, 2.3.1) and the opcode it
// has moved into the instruction register, as the S80C196KC's rings show.
constexpr uint8_t queueBytes(CpuType cpu) {
    return cpu >= CPU_80C196KB ? 5 : 4;
}

struct ArchI8096 final : MatchWalker::Arch {
    explicit ArchI8096(const MatchMemory *mems, CpuType cpu = CPU_8096)
        : Arch(MatchWalker::Traits{.maxStart = 14,
                  .queue = queueBytes(cpu),
                  .cutStart = 7,
                  .leadUnproven = cpu >= CPU_80C196KB,
                  .refetchWord = true,
                  .targetAhead = cpu >= CPU_80C196KB,
                  .pushBeforeVector = InstI8096::pushesBeforeVector(cpu),
                  .oddWordOneCycle = cpu >= CPU_80C196KB,
                  .cutOverlap = cpu >= CPU_80C196KB}),
          _mems(mems),
          _cpu(cpu) {}

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
    const CpuType _cpu;
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
