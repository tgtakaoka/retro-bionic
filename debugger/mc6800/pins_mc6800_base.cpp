#include "pins_mc6800_base.h"
#include "debugger.h"
#include "inst_mc6800.h"
#include "mems_mc6800.h"
#include "regs_mc6800.h"
#include "signals_mc6800.h"

namespace debugger {
namespace mc6800 {

namespace {

inline void assert_irq() {
    digitalWriteFast(PIN_IRQ, LOW);
}

inline void negate_irq() {
    digitalWriteFast(PIN_IRQ, HIGH);
}

}  // namespace

// Open drain: #NMI may also be wired to another source or a switch.
void PinsMc6800Base::assert_nmi() {
    pinMode(PIN_NMI, OUTPUT_OPENDRAIN);
    digitalWriteFast(PIN_NMI, LOW);
}

void PinsMc6800Base::negate_nmi() {
    pinMode(PIN_NMI, INPUT_PULLUP);
}

void PinsMc6800Base::assertInt(uint8_t) {
    assert_irq();
}

void PinsMc6800Base::negateInt(uint8_t) {
    negate_irq();
}

void PinsMc6800Base::setBreakInst(uint32_t addr) const {
    _mems->put_prog(addr, ArchMc6800::SWI);
};

void PinsMc6800Base::printCycles(const Signals *end) {
    const auto g = Signals::get();
    const auto cycles = g->diff(end ? end : Signals::put());
    for (auto i = 0u; i < cycles; ++i) {
        const auto s = g->next(i);
        if (s == end)
            break;
        s->print();
        idle();
    }
}

// The ring's last cycle is the next fetch: ArchMc6800 walks no
// instruction from it.
const Signals *PinsMc6800Base::findFetch(Signals *begin, const Signals *end) {
    _inst->setIdle(
            [](void *pins) { static_cast<PinsMc6800Base *>(pins)->idle(); },
            this);
    auto &walker = MatchWalker::shared();
    if (!walker.walk(*_inst, begin, end))
        return end;
    return begin->next(walker.start());
}

// findFetch() marks the fetch() cycles as it walks the ring, so it runs
// before backtraceStartFrom() counts them; printBacktrace() runs it again
// and derives the same marks.
const SignalsImpl *PinsMc6800Base::findBacktraceStart() {
    const auto end = Signals::put();
    // Make room for idle cycles -- see printBacktrace().
    const auto begin = findFetch(Signals::get()->next(4), end);
    return backtraceStartFrom<Signals>(begin, _lineLimit);
}

void PinsMc6800Base::printBacktrace() {
    const auto end = Signals::put();
#ifdef PROFILE_CYCLES
    // Every cycle, with the marks the matcher gives them; a run's first
    // cycle is its opcode fetch.
    findFetch(Signals::get(), _profileEnd ? _profileEnd : end);
    printCycles(end);
#else
    // Skip the 4 oldest cycles: each idle() writes 4 past the ring's end,
    // over them.
    const auto begin = findFetch(Signals::get()->next(4), end);
    printCycles(begin);
    const auto cycles = begin->diff(end);
    if (Debugger.verbose()) {
        // Every cycle in order, an instruction's line before its fetch.
        for (auto i = 0u; i < cycles; ++i) {
            const auto s = begin->next(i);
            if (s->fetch())
                _mems->disassemble(s->addr, 1);
            s->print();
            idle();
        }
        return;
    }
    const Signals *pref = nullptr;
    for (auto i = 0u; i < cycles;) {
        const auto s = begin->next(i);
        if (pref || s->fetch()) {
            const auto f = pref ? pref : s;
            const auto nexti = _mems->disassemble(f->addr, 1);
            const auto len = nexti - f->addr - (pref ? 1 : 0);
            const auto matched = f->matched();
            pref = nullptr;
            for (auto j = len; j < matched; ++j) {
                const auto p = s->next(j);
                p->print();
                if (p->fetch())
                    pref = p;
            }
            i += matched;
        } else {
            if (s->valid())
                s->print();
            ++i;
        }
        idle();
    }
#endif
}

}  // namespace mc6800
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
