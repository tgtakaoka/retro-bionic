#include "pins_mc6809.h"
#include "debugger.h"
#include "devs_mc6809.h"
#include "digital_bus.h"
#include "inst_mc6809.h"
#include "mems_mc6809.h"
#include "pins_mc6809_config.h"
#include "regs_mc6809.h"
#include "signals_mc6809.h"

namespace debugger {
namespace mc6809 {

/**
 * MC6809 bus cycle.
 *       _    __    __    __    __    __    __    __    __    _
 * EXTAL  |_c|1 |_c|2 |_c|3 |_c|4 |_c|1 |_c|2 |_c|3 |_c|4 |_c|1
 *        \     \_____\_____\     \     \ ____\______\    \
 *     Q __|____|      |     |_____|_____|     |      |____|___
 *       __|           |___________|           |___________|
 *     E   |___________|           |___________|           |___
 *       \     ____________________\                       \
 *   R/W _>---<                     |_______________________>-----
 *                           ____               _________
 *  Data -------------------<rrrr>-------------<wwwwwwwww>-------
 *       _________       _________________       _________________
 *  Addr _________>-----<_________________>-----<_________________>
 */

namespace {

// tCYC: min 1,000 ns ; E/Q cycle
// PWEL: min   430 ns ; e_lo
// PWEH: min   450 ns ; e_hi
// PWQL: min   450 ns ; q_lo
// PWQH: min   430 ns ; q_hi
//  tEQ: min   200 ns ; e to q
//  tAQ: max   200 ns ; addr/ba/bs/rw to q_hi
// tDDQ: max   200 ns ; q_hi to write data
// tDSR: min    80 ns ; data to e_lo
// tPCS: min   200 ns ; reset_hi to q_lo
constexpr auto extal_hi_ns = 105;   // 125
constexpr auto extal_lo_ns = 89;    // 125
constexpr auto c1_lo_ns = 58;       // 125
constexpr auto c1_hi_ns = 66;       // 125
constexpr auto c2_lo_ns = 41;       // 125
constexpr auto c2_hi_ns = 28;       // 125
constexpr auto c3_lo_write = 70;    // 125
constexpr auto c3_hi_write = 75;    // 125
constexpr auto c4_lo_write = 1;     // 125
constexpr auto c4_lo_capture = 75;  // 125
constexpr auto c4_hi_write = 45;    // 125
constexpr auto c3_lo_read = 62;     // 125
constexpr auto c3_hi_read = 7;      // 125
constexpr auto c3_hi_inject = 87;   // 125
constexpr auto c4_lo_read = 55;     // 125
constexpr auto c4_hi_read = 47;     // 125

inline void extal_hi() __attribute__((always_inline));
inline void extal_hi() {
    digitalWriteFast(PIN_EXTAL, HIGH);
}

inline void extal_lo() __attribute__((always_inline));
inline void extal_lo() {
    digitalWriteFast(PIN_EXTAL, LOW);
}

inline uint8_t clock_q() __attribute__((always_inline));
inline uint8_t clock_q() {
    return digitalReadFast(PIN_Q);
}

inline uint8_t clock_e() __attribute__((always_inline));
inline uint8_t clock_e() {
    return digitalReadFast(PIN_E);
}

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

void assert_firq() __attribute__((unused));
void assert_firq() {
    digitalWriteFast(PIN_FIRQ, LOW);
}

void negate_firq() {
    digitalWriteFast(PIN_FIRQ, HIGH);
}

void negate_halt() {
    digitalWriteFast(PIN_HALT, HIGH);
}

void negate_mrdy() {
    digitalWriteFast(PIN_MRDY, HIGH);
}

void negate_breq() {
    digitalWriteFast(PIN_BREQ, HIGH);
}

void assert_reset() {
    // Drive RESET condition
    digitalWriteFast(PIN_RESET, LOW);
    negate_halt();
    negate_nmi();
    negate_irq();
    negate_firq();
    negate_mrdy();
    negate_breq();
}

void negate_reset() {
    // Release RESET conditions
    digitalWriteFast(PIN_RESET, HIGH);
}

constexpr uint8_t PINS_LOW[] = {
        PIN_EXTAL,
        PIN_RESET,
        PIN_MRDY,
};

constexpr uint8_t PINS_HIGH[] = {
        PIN_HALT,
        PIN_IRQ,
        PIN_NMI,
        PIN_FIRQ,
        PIN_BREQ,
};

constexpr uint8_t PINS_PULLUP[] = {
        PIN_XTAL,
};

constexpr uint8_t PINS_INPUT[] = {
        PIN_D0,
        PIN_D1,
        PIN_D2,
        PIN_D3,
        PIN_D4,
        PIN_D5,
        PIN_D6,
        PIN_D7,
        PIN_AL0,
        PIN_AL1,
        PIN_AL2,
        PIN_AL3,
        PIN_AL4,
        PIN_AL5,
        PIN_AL6,
        PIN_AL7,
        PIN_AM8,
        PIN_AM9,
        PIN_AM10,
        PIN_AM11,
        PIN_AH12,
        PIN_AH13,
        PIN_AH14,
        PIN_AH15,
        PIN_RW,
        PIN_Q,
        PIN_E,
        PIN_BS,
        PIN_BA,
};

inline void extal_cycle() {
    extal_hi();
    delayNanoseconds(extal_hi_ns);
    extal_lo();
    delayNanoseconds(extal_lo_ns);
}

}  // namespace

void PinsMc6809::resetPins() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_PULLUP, sizeof(PINS_PULLUP), INPUT_PULLUP);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    assert_reset();
    // Synchronize EXTAL input and Q and E output
    while (true) {
        extal_cycle();
        // Synchoronize to C4L
        if (clock_q() == LOW && clock_e() != LOW)
            break;
    }
    // C4H
    extal_hi();
    delayNanoseconds(extal_hi_ns);
    // C1L
    extal_lo();
    // At least one #RESET cycle.
    cycle();
    cycle();
    negate_reset();
}

void PinsMc6809::reset() {
    resetPins();

    Signals::resetCycles();
    while (!cycle()->vector())
        ;
    // LSB of reset vector;
    cycle();
    cycle();  // non-VMA
    _regs->reset();
    // SoftwareType must be determined before context save.
    _inst->setSoftwareType(_regs->checkSoftwareType());
    _regs->save();
    _regs->setIp(_mems->raw_read16(InstMc6809::VEC_RESET));
}

Signals *PinsMc6809::rawCycle() const {
    // C1H
    extal_hi();
    busMode(D, INPUT);
    auto signals = Signals::put();
    delayNanoseconds(c1_hi_ns);
    // C2L
    extal_lo();
    delayNanoseconds(c2_lo_ns);
    signals->getHighAddr();
    // C2H
    extal_hi();
    signals->getDirection();
    delayNanoseconds(c2_hi_ns);
    // C3L
    extal_lo();
    signals->getLowAddr();
    if (signals->write()) {
        delayNanoseconds(c3_lo_write);
        // C3H
        extal_hi();
        signals->getData();
        delayNanoseconds(c3_hi_write);
        // C4L
        extal_lo();
        if (signals->writeMemory()) {
            _mems->write(signals->addr, signals->data);
            if (c4_lo_write)
                delayNanoseconds(c4_lo_write);
        } else {
            delayNanoseconds(c4_lo_capture);
        }
        // C4H
        extal_hi();
        Signals::nextCycle();
        delayNanoseconds(c4_hi_write);
    } else {
        delayNanoseconds(c3_lo_read);
        // C3H
        extal_hi();
        if (signals->readMemory()) {
            signals->data = _mems->read(signals->addr);
            delayNanoseconds(c3_hi_read);
        } else {
            delayNanoseconds(c3_hi_inject);
        }
        // C4L
        extal_lo();
        busMode(D, OUTPUT);
        busWrite(D, signals->data);
        delayNanoseconds(c4_lo_read);
        // C4H
        extal_hi();
        Signals::nextCycle();
        delayNanoseconds(c4_hi_read);
    }
    // C1L
    signals->clearControl();
    extal_lo();

    return signals;
}

Signals *PinsMc6809::cycle() const {
    delayNanoseconds(c1_lo_ns);
    return rawCycle();
}

Signals *PinsMc6809::injectCycle(uint8_t data) {
    Signals::put()->inject(data);
    return cycle();
}

void PinsMc6809::injectReads(const uint8_t *inst, uint8_t len, uint8_t cycles) {
    for (auto inj = 0; inj < len; ++inj) {
        injectCycle(inst[inj]);
    }
    for (auto inj = len; inj < cycles; ++inj) {
        injectCycle(InstMc6809::NOP);
    }
}

void PinsMc6809::captureWrites(uint8_t *buf, uint8_t len) {
    for (auto cap = 0; cap < len;) {
        auto s = Signals::put();
        s->capture();
        cycle();
        if (s->write())
            buf[cap++] = s->data;
    }
}

uint8_t PinsMc6809::captureContext(uint8_t *context, uint16_t &sp) {
    // 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    // 1:N:x:w:W:W:W:W:W:W:W:W:W:W:W:x:V:v:x     (MC6809)
    // 1:N:x:w:W:W:W:W:W:W:W:W:W:W:W:W:W:x:V:v:x (HD6309 native)
    const uint8_t SWI = 0x3F;
    injectReads(&SWI, sizeof(SWI));
    uint16_t pc = 0;
    uint8_t cap = 0;
    auto s = Signals::put();
    while (!s->vector()) {
        s->capture();
        s->inject(hi(pc));
        s = cycle();
        if (s->write()) {
            if (cap == 0)
                sp = s->addr;
            context[cap++] = s->data;
            pc = uint16(context[1], context[0]);
        }
    }
    Signals::put()->inject(lo(pc));
    cycle();  // SWI lo(vector)
    cycle();  // non-VMA
    return cap;
}

void PinsMc6809::idle() {
    auto s = Signals::put();
    s->inject(InstMc6809::BRA);
    rawCycle();
    injectCycle(InstMc6809::BRA_HERE);
    injectCycle(InstMc6809::NOP);
    Signals::discard(s);
}

const Signals *PinsMc6809::stackFrame(const Signals *push) const {
    while (push->write())
        push = push->prev();
    return push->next();
}

void PinsMc6809::loop() {
    const auto vec_swi = _inst->vec_swi();
    while (true) {
        Devs.loop();
        const auto s = rawCycle();
        if (s->vector() && s->addr == vec_swi) {
            cycle();  // read SWI low(vector);
            cycle();  // non-VMA
            const auto frame = stackFrame(s->prev(2));
            const auto pc = uint16(frame->next()->data, frame->data);
            const auto swi_vector = _mems->raw_read16(vec_swi);
            if (isBreakPoint(pc) || swi_vector == vec_swi) {
                _regs->capture(frame);
                Signals::discard(frame->prev(2));
                return;
            }
        } else if (haltSwitch()) {
            suspend(true);
            return;
        }
    }
}

void PinsMc6809::suspend(bool show) {
    assert_nmi();
reentry:
    auto s = Signals::put();
    while (true) {
        s = cycle();
        if (s->vector())  // NMI hi(vector)
            break;
    }
    negate_nmi();
    if (s->addr == _inst->vec_swi()) {
        cycle();  // SWI lo(vector)
        cycle();  // non-VMA
        goto reentry;
    }
    const auto frame = stackFrame(s->prev(2));
    cycle();  // NMI lo(vector)
    cycle();  // non-VMA
    _regs->capture(frame, true);
    if (show) {
        const auto last = frame->prev(_regs->contextLength() == 14 ? 3 : 2);
        Signals::discard(last);
    }
}

void PinsMc6809::run() {
    _regs->restore();
    Signals::resetCycles();
    saveBreakInsts();
    loop();
    restoreBreakInsts();
    disassembleCycles();
}

bool PinsMc6809::step(bool show) {
    Signals::resetCycles();
    _regs->restore();
    if (show)
        Signals::resetCycles();
    suspend(show);
    if (show)
        printCycles(Signals::put());
    return true;
}

void PinsMc6809::assertInt(uint8_t name) {
    if (name == INTR_IRQ)
        assert_irq();
    if (name == INTR_FIRQ)
        assert_firq();
}

void PinsMc6809::negateInt(uint8_t name) {
    if (name == INTR_IRQ)
        negate_irq();
    if (name == INTR_FIRQ)
        negate_firq();
}

void PinsMc6809::setBreakInst(uint32_t addr) const {
    _mems->put_inst(addr, InstMc6809::SWI);
}

void PinsMc6809::printCycles(const Signals *end) {
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

bool PinsMc6809::matchAll(Signals *begin, const Signals *end) {
    const auto cycles = begin->diff(end->prev());
    LOG_MATCH(cli.print("@@  matchAll: begin="));
    LOG_MATCH(begin->print());
    LOG_MATCH(cli.print("@@           cycles="));
    LOG_MATCH(cli.printlnDec(cycles));
    for (auto i = 0; i < cycles;) {
        idle();
        auto s = begin->next(i);
        if (_inst->match(s, end, nullptr)) {
            s->markFetch(_inst->matched());
            for (auto m = 1; m < _inst->matched(); ++m)
                s->next(m)->clearFetch();
            i += _inst->matched();
            continue;
        }
        idle();
        if (_inst->matchInterrupt(s, end)) {
            for (auto m = 1; m < _inst->matched(); ++m)
                s->next(m)->clearFetch();
            i += _inst->matched();
            continue;
        }
        return false;
    }
    return true;
}

const Signals *PinsMc6809::findFetch(Signals *begin, const Signals *end) {
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

void PinsMc6809::disassembleCycles() {
    const auto end = Signals::put();
    // Make room for idle cycles.
    const auto begin = findFetch(Signals::get()->next(3), end);
    printCycles(begin);
    const auto cycles = begin->diff(end);
    for (auto i = 0; i < cycles;) {
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
}

}  // namespace mc6809
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
