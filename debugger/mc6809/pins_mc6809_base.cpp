#include "pins_mc6809_base.h"
#include "debugger.h"
#include "devs_mc6809.h"
#include "digital_bus.h"
#include "inst_mc6809.h"
#include "mems_mc6809.h"
#include "regs_mc6809.h"
#include "signals_mc6809.h"

namespace debugger {
namespace mc6809 {

namespace {
void assert_nmi() {
    digitalWriteFast(PIN_NMI, LOW);
}

void negate_nmi() {
    digitalWriteFast(PIN_NMI, HIGH);
}

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

void PinsMc6809Base::reset() {
    resetPins();
    Signals::resetCycles();

    while (!cycle()->vector())
        ;
    // LSB of reset vector;
    cycle();
    cycle();  // non-VMA
    _regs.reset();
    // SoftwareType must be determined before context save.
    _inst.setSoftwareType(_regs.checkSoftwareType());
    _regs.save();
    _regs.setIp(_mems.raw_read16(InstMc6809::VEC_RESET));
}

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

void PinsMc6809Base::idle() {
    auto s = Signals::put();
    s->inject(InstMc6809::BRA);
    rawCycle();
    injectCycle(InstMc6809::BRA_HERE);
    injectCycle(InstMc6809::NOP);
    Signals::discard(s);
}

const Signals *PinsMc6809Base::stackFrame(const Signals *push) const {
    while (push->write())
        push = push->prev();
    return push->next();
}

void PinsMc6809Base::loop() {
    const auto vec_swi = _inst.vec_swi();
    while (true) {
        Devs.loop();
        const auto s = rawCycle();
        if (s->vector() && s->addr == vec_swi) {
            cycle();  // read SWI low(vector);
            cycle();  // non-VMA
            const auto frame = stackFrame(s->prev(2));
            const auto pc = uint16(frame->next()->data, frame->data);
            const auto swi_vector = _mems.raw_read16(vec_swi);
            if (isBreakPoint(pc) || swi_vector == vec_swi) {
                _regs.capture(frame);
                Signals::discard(frame->prev(2));
                return;
            }
        } else if (haltSwitch()) {
            suspend(true);
            return;
        }
    }
}

void PinsMc6809Base::suspend(bool show) {
    assert_nmi();
reentry:
    auto s = Signals::put();
    while (true) {
        s = cycle();
        if (s->vector())  // NMI hi(vector)
            break;
    }
    negate_nmi();
    if (s->addr == _inst.vec_swi()) {
        cycle();  // SWI lo(vector)
        cycle();  // non-VMA
        goto reentry;
    }
    const auto frame = stackFrame(s->prev(2));
    cycle();  // NMI lo(vector)
    cycle();  // non-VMA
    _regs.capture(frame, true);
    if (show) {
        // Keep prefetch cycle for bus cycle pattern matching.
        const auto last = frame->prev(_regs.contextLength() == 14 ? 3 : 2);
        Signals::discard(last);
    }
}

void PinsMc6809Base::run() {
    _regs.restore();
    Signals::resetCycles();
    saveBreakInsts();
    loop();
    restoreBreakInsts();
    disassembleCycles();
}

bool PinsMc6809Base::step(bool show) {
    Signals::resetCycles();
    _regs.restore();
    if (show)
        Signals::resetCycles();
    suspend(show);
    if (show) {
        // Discard prefetch cycle.
        const auto end = Signals::put()->prev(1);
        printCycles(end);
    }
    return true;
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
    _mems.put_inst(addr, InstMc6809::SWI);
}

void PinsMc6809Base::printCycles(const Signals *end) {
    const auto g = Signals::get();
    const auto cycles = g->diff(end ? end : Signals::put());
    for (auto i = 0; i < cycles; ++i) {
        const auto s = g->next(i);
        if (s == end)
            break;
        s->print();
        idle();
    }
}

bool PinsMc6809Base::matchAll(Signals *begin, const Signals *end) {
    const auto cycles = begin->diff(end->prev());
    LOG_MATCH(cli.print("@@  matchAll: begin="));
    LOG_MATCH(begin->print());
    LOG_MATCH(cli.print("@@           cycles="));
    LOG_MATCH(cli.printlnDec(cycles));
    for (auto i = 0; i < cycles;) {
        idle();
        auto s = begin->next(i);
        if (_inst.match(s, end, nullptr)) {
            s->markFetch(_inst.matched());
            for (auto m = 1; m < _inst.matched(); ++m)
                s->next(m)->clearFetch();
            i += _inst.matched();
            continue;
        }
        idle();
        if (_inst.matchInterrupt(s, end)) {
            for (auto m = 1; m < _inst.matched(); ++m)
                s->next(m)->clearFetch();
            i += _inst.matched();
            continue;
        }
        return false;
    }
    return true;
}

const Signals *PinsMc6809Base::findFetch(Signals *begin, const Signals *end) {
    const auto cycles = begin->diff(end);
    const auto limit = cycles < 40 ? cycles : 40;
    LOG_MATCH(cli.print("@@ findFetch: begin="));
    LOG_MATCH(begin->print());
    LOG_MATCH(cli.print("@@              end="));
    LOG_MATCH(end->print());
    for (auto i = 0; i < limit; ++i) {
        auto s = begin->next(i);
        if (matchAll(s, end))
            return s;
        idle();
        for (auto j = i; j < cycles; ++j)
            begin->next(j)->clearFetch();
    }
    return end;
}

void PinsMc6809Base::disassembleCycles() {
    const auto end = Signals::put();
    // Make room for idle cycles.
    const auto begin = findFetch(Signals::get()->next(3), end);
    printCycles(begin);
    const auto cycles = begin->diff(end);
    for (auto i = 0; i < cycles;) {
        const auto s = begin->next(i);
        if (s->fetch()) {
            const auto len = _mems.disassemble(s->addr, 1) - s->addr;
            i += len;
        } else {
            s->print();
            ++i;
        }
        idle();
    }
}

}  // namespace mc6809
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
