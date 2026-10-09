#ifndef __DEBUGGER_MATCH_WALKER_H__
#define __DEBUGGER_MATCH_WALKER_H__

#include <stdint.h>

#include "cycles.h"
#include "signals.h"

namespace debugger {

// Splits a ring of bus cycles into instructions. From each start it
// decodes an instruction, matches its sequence (match_legend.md) against
// the cycles, and goes on at the next instruction's fetch; an alternative
// that fails later sends it back to try the next. A start's walk must
// reach the ring's end: at the stop PC when it's known, where any does.
// The ring may end inside an instruction, where a halt cut it, and begin
// inside one. A walk needs a next fetch seen inside the ring to confirm
// it, but from the ring's first cycle where its last isn't a next fetch;
// one that isn't goes back to try the next alternative. Where a prefetch
// queue fetches ahead, the next instruction's first bytes may come between
// an instruction's own cycles. What a cycle is, what an instruction's
// sequence is and how marks are kept are each CPU's own: its Arch.
struct MatchWalker {
    // What a bus cycle is.
    enum Kind : uint8_t {
        K_NONE = 0,  // no valid address
        K_READ = 1,
        K_WRITE = 2,
        K_IO_READ = 3,
        K_IO_WRITE = 4,
        K_ACK = 5,   // an interrupt acknowledge
        K_HALT = 6,  // halted
    };

    // What the walk made of a cycle, for Arch::markCycle().
    enum Role : uint8_t {
        R_NONE = 0,   // no instruction's
        R_FETCH = 1,  // an instruction's first byte
        R_BYTE = 2,   // an instruction's other bytes
        R_DATA = 3,   // a data cycle
    };

    // An instruction decoded at |pc|: its sequence, its length, and the
    // target and effective address its bytes give, where they do. Not
    // copied: |seq| may point into its own |text|.
    struct Decoded {
        Decoded() = default;
        Decoded(const Decoded &) = delete;
        Decoded &operator=(const Decoded &) = delete;
        uint32_t pc;
        const char *seq;  // a table's, or text
        char text[48];    // for a decoder that puts one together
        uint8_t length;
        bool hasTarget;
        bool hasEa;
        uint32_t target;
        uint32_t ea;
    };

    // How a CPU's bus puts a value together, and how its rings are kept;
    // a CPU names the ones it sets: Traits{.bigEndian = true, ...}, in
    // this order.
    struct Traits {
        bool bigEndian = false;
        // what a P fetch adds to the value: 1 where the PC is incremented
        // before a fetch
        uint8_t targetBias = 0;
        uint32_t addressMask = 0xFFFF;
        // the cycles a start may be among, from the ring's first; 0: all
        uint8_t maxStart = 0;
        // the ring's last cycle is the next instruction's fetch: no
        // instruction is walked from it, and none the ring cuts took a
        // cycle of its own past its fetch
        bool lastIsNextFetch = false;
        // the instruction bytes a prefetch queue holds, past ~; 1 where a
        // next fetch can only be handed over
        uint8_t queue = 1;
        // how far back a stall may fetch again an instruction's bytes the
        // fetch stream has already fetched, anywhere; 0: it doesn't
        uint8_t refetch = 0;
        // the bytes the fetch stream may have passed before the ring's
        // first cycle: an instruction it began in, whose data is in it
        uint8_t cutStart = 0;
        // the ring's first fetches may be the queue's: its first
        // instructions count only from one that moves data, transfers or
        // takes an interrupt
        bool leadUnproven = false;
        // a word fetch takes the bytes the queue has room for, and the
        // word may be read again: for the rest, or at once
        bool refetchWord = false;
    };

    // A CPU, as the walk sees it.
    struct Arch {
        explicit Arch(const Traits &t) : traits(t) {}
        const Traits traits;

        virtual Kind cycleKind(const SignalsImpl *s) const = 0;
        // false where there is no instruction
        virtual bool decode(uint32_t pc, Decoded &inst) const = 0;
        // V: whether |addr| is in the vector table
        virtual bool isVectorTable(uint32_t) const { return false; }
        // x: whether |s| is the CPU's dummy read
        virtual bool isDummy(const SignalsImpl *) const { return false; }
        // The bytes a fetch brings, from its address.
        virtual uint_fast8_t fetchBytes(const SignalsImpl *) const { return 1; }
        // The bytes a data cycle moves, and each in address order.
        virtual uint_fast8_t dataBytes(const SignalsImpl *) const { return 1; }
        virtual uint8_t dataByte(const SignalsImpl *s, uint_fast8_t) const {
            return s->data;
        }
        // Whether a fetch from |bus| is one from |addr|, and whether the
        // CPU is at |stop|: where an MMU maps |addr|, not only the same.
        virtual bool sameAddress(uint32_t bus, uint32_t addr) const {
            return bus == addr;
        }
        // Whether |s| is an interrupt's entry, fetched after the
        // acknowledge |ack|, if any.
        virtual bool entersInterrupt(
                const SignalsImpl *, const SignalsImpl *) const {
            return true;
        }
        // Whether an interrupt's first write |s| pushes the PC: |next|,
        // where it would have gone on, or for a trap |last|, the
        // instruction it aborts.
        virtual bool pushesPc(const SignalsImpl *, uint32_t, uint32_t) const {
            return true;
        }
        // what an interrupt does, tried where no instruction fits; none
        // if null
        virtual const char *interruptSequence() const { return nullptr; }
        // how a wait ends, its cycles then the interrupt's vector and
        // fetch, tried at the ring's first cycle where the ring began
        // inside one; none if null
        virtual const char *resumeSequence() const { return nullptr; }
        // Called as the walk goes, to keep the board's bus alive.
        virtual void idle() const {}
        // |span|: on R_FETCH, the cycles the instruction took
        virtual void markCycle(
                SignalsImpl *s, Role role, uint_fast8_t span) const = 0;

    protected:
        ~Arch() = default;
    };

    // The instruction bytes, 1 and 2, in the first alternative of |seq|.
    static uint_fast8_t instructionBytes(const char *seq);

#ifndef ARDUINO
    // Why |seq| isn't a sequence of match_legend.md's tokens, |own| those
    // a decoder expands before the walk; nullptr where it is. An
    // instruction's alternatives are all as long; an interrupt's needn't.
    static const char *illFormed(
            const char *seq, const char *own = "", bool instruction = true);
#endif

    // walk()'s |stop| when the PC isn't known: the ring may end anywhere.
    static constexpr uint32_t NO_STOP = UINT32_MAX;

    // The walk of the ring [begin, end) from its earliest start that
    // reaches the end, leaving the CPU at |stop|. Marks every cycle
    // through Arch::markCycle(); returns false when no start walks. It
    // recurses an instruction at a time, a few hundred bytes of stack
    // each.
    bool walk(const Arch &arch, SignalsImpl *begin, const SignalsImpl *end,
            uint32_t stop = NO_STOP);

    // The cycles an instruction starting at |begin| takes, its next fetch
    // before |end|; 0 if it doesn't match there.
    uint_fast8_t matchInstruction(
            const Arch &arch, SignalsImpl *begin, const SignalsImpl *end);

    // Big: one for the board, whichever CPU it has.
    static MatchWalker &shared();

    // After walk(): the cycle it starts from, and its instructions.
    uint_fast8_t start() const { return _start; }
    uint_fast16_t steps() const { return _steps; }
    uint32_t addr(uint_fast16_t step) const { return _step[step].pc; }
    uint_fast8_t first(uint_fast16_t step) const { return _step[step].first; }
    uint_fast8_t cycles(uint_fast16_t step) const { return _step[step].cycles; }
    // an interrupt taken: no instruction at addr()
    bool interrupt(uint_fast16_t step) const { return _step[step].interrupt; }
    // The step whose cycles the cycle |i| is among, NOBODY if none.
    uint_fast16_t owner(uint_fast8_t i) const;
    static constexpr uint_fast16_t NOBODY = UINT16_MAX;

#ifndef ARDUINO
    static bool trace;  // host test: print the walk's failures
#endif

private:
    // An instruction walked, or an interrupt taken: its address, its first
    // cycle, how many cycles it took, and the cycle that fetched it.
    struct Step {
        uint32_t pc;
        uint8_t first;
        uint8_t cycles;
        uint8_t fetchAt;   // NO_FETCH: none, as for most interrupts
        bool sawNext : 1;  // its next fetch is in the ring
        bool interrupt : 1;
        bool moved : 1;  // it moved data or transferred
    };
    // no cycle: a cycle's index is a byte
    static constexpr uint8_t NO_FETCH = 0xFF;
    static_assert(Cycles::MAX_CYCLES < NO_FETCH, "a cycle's index is a byte");
    // The cycles that fetched the next instruction's first bytes ahead.
    struct Queue {
        static constexpr uint8_t MAX = 8;
        uint8_t at[MAX];
        uint8_t n;
        uint32_t bias;  // where the fetch stream is on the bus: + the PC
        void push(uint8_t i) { at[n++] = i; }
        uint8_t pop() {
            const auto head = at[0];
            for (auto k = 1u; k < n; ++k)
                at[k - 1] = at[k];
            --n;
            return head;
        }
    };
    // A match's state while it goes through one alternative.
    struct Match;

    const Arch *_arch;
    SignalsImpl *_begin;
    uint_fast8_t _size;
    uint32_t _stop;
    uint_fast8_t _from;     // the start being walked
    bool _before;           // it began before the ring
    uint_fast8_t _written;  // how far an alternative's match marked
    uint32_t _budget;
    uint_fast8_t _start;
    // a fetch may bring two instructions, and the queue more
    static constexpr uint_fast16_t MAX_STEPS =
            2 * Cycles::MAX_CYCLES + Queue::MAX;
    uint_fast16_t _steps;
    Step _step[MAX_STEPS];
    Role _role[Cycles::MAX_CYCLES];
    Kind _kind[Cycles::MAX_CYCLES];  // each cycle's, from Arch::cycleKind()

    SignalsImpl *at(uint_fast8_t i) const { return _begin->_next(i); }
    void classify();
    bool walked(uint_fast8_t i);
    bool endsAt(uint32_t pc, uint32_t stream) const;
    bool endsAt(uint32_t pc) const { return endsAt(pc, pc); }
    bool ranTo(uint32_t pc, const Queue &fetched);
    bool atStop(uint32_t pc) const;
    bool confirmed() const;
    bool cutAt(uint8_t fetchAt, uint_fast8_t cycles) const;
    bool walkFrom(uint_fast8_t i, uint32_t pc, const Queue &queued);
    bool walkOn(const Decoded &inst, uint_fast8_t i, const Queue &queued,
            bool interrupt);
    bool matchAlternative(const Decoded &inst, const char *seq, const char *end,
            const Queue &queued, Match &m);
};

}  // namespace debugger
#endif /* __DEBUGGER_MATCH_WALKER_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
