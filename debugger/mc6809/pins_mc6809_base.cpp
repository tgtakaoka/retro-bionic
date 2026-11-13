#include "pins_mc6809_base.h"
#include "debugger.h"
#include "inst_mc6809.h"
#include "regs_mc6809.h"
#include "signals_mc6809.h"

namespace debugger {
namespace mc6809 {

namespace {
void assert_irq() {
    digitalWriteFast(PIN_IRQ, LOW);
}

void negate_irq() {
    digitalWriteFast(PIN_IRQ, HIGH);
}

void assert_firq() {
    digitalWriteFast(PIN_FIRQ, LOW);
}

void negate_firq() {
    digitalWriteFast(PIN_FIRQ, HIGH);
}

}  // namespace

Signals *PinsMc6809Base::injectCycle(uint8_t data) {
    Signals::put()->inject(data);
    return cycle();
}

void PinsMc6809Base::injectReads(
        const uint8_t *inst, uint8_t len, uint8_t cycles) {
    for (auto inj = 0; inj < len; ++inj) {
        injectCycle(inst[inj]);
    }
    for (auto inj = len; inj < cycles; ++inj) {
        injectCycle(InstMc6809::NOP);
    }
}

void PinsMc6809Base::captureWrites(uint8_t *buf, uint8_t len) {
    for (auto cap = 0; cap < len;) {
        auto s = Signals::put();
        s->capture();
        cycle();
        if (s->write())
            buf[cap++] = s->data;
    }
}

uint8_t PinsMc6809Base::captureContext(uint8_t *context, uint16_t &sp) {
    // 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    // 1:N:x:w:W:W:W:W:W:W:W:W:W:W:W:x:V:v:x     (MC6809)
    // 1:N:x:w:W:W:W:W:W:W:W:W:W:W:W:W:W:x:V:v:x (HD6309 native)
    const uint8_t SWI = 0x3F;
    injectReads(&SWI, sizeof(SWI));
    uint16_t pc = 0;
    uint8_t cap = 0;
    const Signals *s;
    // Feed the captured PC as the SWI vector, so the CPU resumes at it.
    do {
        s = Signals::put()->inject(hi(pc))->capture();
        cycle();
        if (s->write()) {
            if (cap == 0)
                sp = s->addr;
            context[cap++] = s->data;
            pc = uint16(context[1], context[0]);
        }
    } while (!s->vector());
    Signals::put()->inject(lo(pc));
    cycle();  // SWI lo(vector)
    cycle();  // non-VMA
    return cap;
}

// BRA * clocks the CPU without moving it, then its cycles are discarded.
void PinsMc6809Base::idle() {
    auto s = Signals::put();
    s->inject(InstMc6809::BRA);
    rawCycle();
    injectCycle(InstMc6809::BRA_HERE);
    injectCycle(InstMc6809::NOP);
    Cycles::discard(s);
}

// The first write of the context push that ends at |push|.
const Signals *PinsMc6809Base::stackFrame(const Signals *push) const {
    while (push->write())
        push = push->prev();
    return push->next();
}

void PinsMc6809Base::assertInt(uint8_t name) {
    if (name == INTR_IRQ)
        assert_irq();
    if (name == INTR_FIRQ)
        assert_firq();
}

void PinsMc6809Base::negateInt(uint8_t name) {
    if (name == INTR_IRQ)
        negate_irq();
    if (name == INTR_FIRQ)
        negate_firq();
}

void PinsMc6809Base::setBreakInst(uint32_t addr) const {
    _mems->put_prog(addr, InstMc6809::SWI);
}

void PinsMc6809Base::printCycles(const Signals *end) {
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

const Signals *PinsMc6809Base::findFetch(Signals *begin, const Signals *end) {
    _inst->setIdle(
            [](void *pins) { static_cast<PinsMc6809Base *>(pins)->idle(); },
            this);
    auto &walker = MatchWalker::shared();
    if (!walker.walk(*_inst, begin, end))
        return end;
    return begin->next(walker.start());
}

// fetch() isn't live on either board: findFetch() marks it while matching
// instructions against the captured cycles, so it must run before
// backtraceStartFrom() counts fetches.
const SignalsImpl *PinsMc6809Base::findBacktraceStart() {
    const auto end = Signals::put();
    // Skip the oldest three cycles, which idle()'s BRA * can overwrite
    // in a full ring.
    const auto begin = findFetch(Signals::get()->next(3), end);
    return backtraceStartFrom<Signals>(begin, _lineLimit);
}

void PinsMc6809Base::printBacktrace() {
    const auto end = Signals::put();
#ifdef PROFILE_CYCLES
    // Every cycle, with the marks the matcher gives them; a run's first
    // cycle is its opcode fetch.
    findFetch(Signals::get(), _profileEnd ? _profileEnd : end);
    printCycles(end);
#else
    // Skip what idle() can overwrite, as findBacktraceStart() does.
    const auto begin = findFetch(Signals::get()->next(3), end);
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
    for (auto i = 0u; i < cycles;) {
        const auto s = begin->next(i);
        if (s->fetch()) {
            const auto len = _mems->disassemble(s->addr, 1) - s->addr;
            i += len;
        } else {
            s->print();
            ++i;
        }
        idle();
    }
#endif
}

}  // namespace mc6809
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
