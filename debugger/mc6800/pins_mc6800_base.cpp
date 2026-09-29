#include "pins_mc6800_base.h"
#include "debugger.h"
#include "inst_mc6800.h"
#include "mems_mc6800.h"
#include "regs_mc6800.h"
#include "signals_mc6800.h"

namespace debugger {
namespace mc6800 {

namespace {

// Longest instruction plus a context save, with margin.
constexpr auto SUSPEND_CYCLES = 64;

inline void assert_irq() {
    digitalWriteFast(PIN_IRQ, LOW);
}

inline void negate_irq() {
    digitalWriteFast(PIN_IRQ, HIGH);
}

// #NMI may be connected to other signal or switch
void assert_nmi() {
    pinMode(PIN_NMI, OUTPUT_OPENDRAIN);
    digitalWriteFast(PIN_NMI, LOW);
}

void negate_nmi() {
    pinMode(PIN_NMI, INPUT_PULLUP);
}

}  // namespace

Signals *PinsMc6800Base::injectCycle(uint8_t data) {
    Signals::put()->inject(data);
    return cycle();
}

void PinsMc6800Base::injectReads(
        const uint8_t *inst, uint8_t len, uint8_t cycles) {
    for (auto inj = 0; inj < len; ++inj) {
        injectCycle(inst[inj]);
    }
    for (auto inj = len; inj < cycles; ++inj) {
        injectCycle(InstMc6800::NOP);
    }
}

void PinsMc6800Base::captureWrites(uint8_t *buf, uint8_t len, uint16_t *addr) {
    for (auto cap = 0; cap < len;) {
        auto s = Signals::put();
        s->capture();
        cycle();
        if (s->write()) {
            if (cap == 0 && addr)
                *addr = s->addr;
            buf[cap++] = s->data;
        }
    }
}

void PinsMc6800Base::idle() {
    auto s = Signals::put();
    s->inject(InstMc6800::BRA);
    rawCycle();
    injectCycle(InstMc6800::BRA_HERE);
    injectCycle(InstMc6800::NOP);
    injectCycle(InstMc6800::NOP);
    Cycles::discard(s);
}

#ifdef PROFILE_CYCLES
// For scripts/record-cycles.py: stop at any SWI, keep every cycle of it,
// and give up well before the ring wraps; the debug pin frames the run, to
// trigger a capture on.
void PinsMc6800Base::loop() {
    constexpr auto MAX_CYCLES = 96;
    _profileEnd = nullptr;
    assert_debug();
    for (auto n = 0;; ++n) {
        _devs->loop();
        rawCycle();
        if (_writes == regs<RegsMc6800>()->contextLength()) {
            const auto frame = Signals::put()->prev(_writes);
            if (nonVmaAfterContextSave())
                cycle();                  // non VMA cycle
            const auto vec_hi = cycle();  // read interrupt high(vector)
            if (vec_hi->addr == _inst->vec_swi()) {
                negate_debug();
                cycle();  // read interrupt low(vector)
                regs<RegsMc6800>()->capture(frame, false);
                // Match up to the SWI's opcode fetch, as the normal build does.
                _profileEnd = frame->prev(nonVmaAfterContextSave() ? 1 : 2);
                return;
            }
            _writes = 0;
        } else if (n >= MAX_CYCLES || haltSwitch()) {
            negate_debug();
            cli.println(n >= MAX_CYCLES ? "?cycles" : "?halt");
            suspend(true);
            return;
        }
    }
}
#else
void PinsMc6800Base::loop() {
    while (true) {
        _devs->loop();
        rawCycle();
        if (_writes == regs<RegsMc6800>()->contextLength()) {
            const auto frame = Signals::put()->prev(_writes);
            if (nonVmaAfterContextSave())
                cycle();                  // non VMA cycle
            const auto vec_hi = cycle();  // read interrupt high(vector)
            const auto vec_swi = _inst->vec_swi();
            if (vec_hi->addr == vec_swi) {
                cycle();  // read interrupt low(vector)
                const auto pc = regs<RegsMc6800>()->capture(frame, false);
                const auto swi_vector = _mems->read16(vec_swi);
                if (isBreakPoint(pc) || swi_vector == vec_swi) {
                    const auto discard = nonVmaAfterContextSave() ? 1 : 2;
                    Cycles::discard(frame->prev(discard));
                    return;
                }
            }
            // WAI idles on non-VMA cycles, which leave _writes as is.
            _writes = 0;
        } else if (haltSwitch()) {
            suspend(true);
            return;
        }
    }
}
#endif

void PinsMc6800Base::suspend(bool show) {
    assert_nmi();
reentry:
    _writes = 0;
    // Wait for consecutive writes which means registers saved onto stack.
    for (auto n = 0; _writes < regs<RegsMc6800>()->contextLength(); ++n) {
        const auto s = cycle();
        if (s->valid() && s->read() && s->addr == _inst->vec_nmi()) {
            // In WAI the context is already stacked, so NMI fetches its
            // vector without pushing. RTI back out of WAI, then take a
            // fresh NMI edge.
            negate_nmi();
            cycle();  // NMI lo(vector)
            injectCycle(InstMc6800::RTI);
            assert_nmi();
            goto reentry;
        }
        if (n >= SUSPEND_CYCLES) {
            negate_nmi();
            cli.println("?halt: no context saved");
            return;
        }
    }
    // Capture registers pushed onto stack.
    const auto frame = Signals::put()->prev(_writes);
    // A push by WAI is followed by idle cycles until the NMI is taken,
    // so find the vector read rather than counting cycles to it.
    const Signals *v;
    for (auto n = 0;; ++n) {
        v = cycle();
        if (v->valid() && v->read() &&
                (v->addr == _inst->vec_swi() || v->addr == _inst->vec_nmi()))
            break;
        if (n >= SUSPEND_CYCLES) {
            negate_nmi();
            cli.println("?halt: no vector fetched");
            return;
        }
    }
    negate_nmi();
    if (v->addr == _inst->vec_swi()) {
        assert_nmi();
        cycle();  // SWI lo(vector);
        goto reentry;
    }
    cycle();  // NMI lo(vector)
    regs<RegsMc6800>()->capture(frame);
    if (show) {
        const auto discard = nonVmaAfterContextSave() ? 1 : 2;
        Cycles::discard(frame->prev(discard));
    }
}

void PinsMc6800Base::run() {
    _regs->restore();
    Cycles::reset();
    saveBreakInsts();
    startRunTimer();
    loop();
    stopRunTimer();
    restoreBreakInsts();
    disassembleCycles();
}

bool PinsMc6800Base::step(bool show) {
    Cycles::reset();
    _regs->restore();
    if (show)
        Cycles::reset();
    suspend(show);
    if (show)
        printCycles(Signals::put()->prev());
    return true;
}

void PinsMc6800Base::assertInt(uint8_t) {
    assert_irq();
}

void PinsMc6800Base::negateInt(uint8_t) {
    negate_irq();
}

void PinsMc6800Base::setBreakInst(uint32_t addr) const {
    _mems->put_prog(addr, InstMc6800::SWI);
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

bool PinsMc6800Base::matchAll(Signals *begin, const Signals *end) {
    const auto cycles = begin->diff(end->prev());
    LOG_MATCH(cli.print("@@  matchAll: begin="));
    LOG_MATCH(begin->print());
    LOG_MATCH(cli.print("@@           cycles="));
    LOG_MATCH(cli.printlnDec(cycles));
    Signals *prefetch = nullptr;
    for (auto i = 0u; i < cycles;) {
        idle();
        const auto s = begin->next(i);
        if (prefetch == s)
            prefetch = nullptr;
        if (_inst->match(s, end, prefetch)) {
            const auto f = prefetch ? prefetch : s;
            f->markFetch(_inst->matched());
            const auto nexti = _inst->nextInstruction();
            prefetch = (nexti < 0) ? nullptr : s->next(nexti);
            i += _inst->matched();
            continue;
        }
        idle();
        if (_inst->matchInterrupt(s, end)) {
            for (auto m = 1; m < _inst->matched(); ++m)
                s->next(m)->clearFetch();
            i += _inst->matched();
            prefetch = nullptr;
            continue;
        }
        return false;
    }
    return true;
}

const Signals *PinsMc6800Base::findFetch(Signals *begin, const Signals *end) {
    const auto cycles = begin->diff(end);
    // maximum instruction clock is 13.
    const auto limit = cycles < 13 ? cycles : 13;
    LOG_MATCH(cli.print("@@ findFetch: begin="));
    LOG_MATCH(begin->print());
    LOG_MATCH(cli.print("@@              end="));
    LOG_MATCH(end->print());
    for (auto i = 0u; i < limit; ++i) {
        for (auto j = i; j < cycles; ++j)
            begin->next(j)->clearFetch();
        auto s = begin->next(i);
        if (matchAll(s, end))
            return s;
        idle();
    }
    for (auto j = 0u; j < cycles; ++j)
        begin->next(j)->clearFetch();
    return end;
}

// fetch() isn't live here: findFetch()/matchAll() mark it only as a
// side effect of matching decoded instructions against captured bus
// cycles, so that has to run -- once, right here -- before
// backtraceStartFrom() counting fetch() cycles means anything.
// printBacktrace() re-runs it over the now-disposed range, which
// re-derives the same marks.
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
    // Make room for idle cycles.
    const auto begin = findFetch(Signals::get()->next(4), end);
    printCycles(begin);
    const auto cycles = begin->diff(end);
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
