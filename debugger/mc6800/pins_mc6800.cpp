#include "pins_mc6800.h"
#include "debugger.h"
#include "devs_mc6800.h"
#include "digital_bus.h"
#include "inst_mb8861.h"
#include "inst_mc6800.h"
#include "mems_mc6800.h"
#include "pins_mc6800_config.h"
#include "regs_mc6800.h"
#include "signals_mc6800.h"

namespace debugger {
namespace mc6800 {

/**
 * MC6800 bus cycle.
 *         __________            __________            _____
 *  PHI1 _|          |__________|          |__________|
 *       _     _________________     _________________     _
 *   DBE  |___|                 |___|                 |___|
 *       _            __________            __________
 *  PHI2  |__________|          |__________|          |_____
 *       ________  ____________________  ___________________
 *   VMA ________><____________________><___________________
 *       ________  ____________________
 *   R/W ________>                     |____________________
 *       __       _______________       _______________
 *  Addr __>-----<_______________>-----<_______________>----
 *                           ____             _________
 *  Data -------------------<____>-----------<_________>----
 *
 * - Minimum DBE asserted period is 150ns.
 * - Minimum PHI1/PHI2 high period is 400ns.
 * - Minimum overrapping of PHI1 and PHI2 is 0ns.
 * - Maximum separation of PHI1 and PHI2 is 9100ns.
 * - PHI1 rising-edge to valid VMA, R/W and address is 270ns.
 * - Read data setup to falling PHI2 egde is 100ns.
 * - Read data hold to falling PHI2 edge is 10ns.
 * - Write data gets valid after 225ns of rising DBE edge.
 */

namespace {

// tCYC: min 1000 ns
// td:   max 9100 ns; phi1_lo and phi2_lo
// tPWH: min  400 ns; phi1_hi, phi2_hi
// tUT:  min  900 ns; phi1_hi + phi2_hi
// tAD:  max  270 ns; phi1_hi to addr
// tDSR: min  100 ns; data to phi2_lo
// tDDW: max  225 ns; dbe_hi to data
// tDBE: min  150 ns; dbe_lo
// tPCS: min  200 ns; reset_hi to phi2_lo
constexpr auto dbe_lo_ns = 108;        // 150 ns
constexpr auto phi1_hi_ns = 140;       // 400 ns
constexpr auto phi2_hi_novma = 396;    // 500 ns
constexpr auto phi2_hi_write = 266;    // 500 ns
constexpr auto phi2_hi_read = 238;     // 500 ns
constexpr auto phi2_hi_capture = 362;  // 500 ns
constexpr auto phi2_hi_inject = 68;    // 500 ns
constexpr auto tpcs_ns = 200;

inline void phi1_hi() __attribute__((always_inline));
inline void phi1_hi() {
    digitalWriteFast(PIN_PHI1, HIGH);
    digitalWriteFast(PIN_DBE, LOW);
}

inline void phi2_hi() __attribute__((always_inline));
inline void phi2_hi() {
    digitalWriteFast(PIN_PHI1, LOW);
    digitalWriteFast(PIN_PHI2, HIGH);
}

inline void phi2_lo() __attribute__((always_inline));
inline void phi2_lo() {
    digitalWriteFast(PIN_PHI2, LOW);
}

inline void assert_dbe() __attribute__((always_inline));
inline void assert_dbe() {
    digitalWriteFast(PIN_DBE, HIGH);
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

// TODO: Utilize #HALT to Pins.step()
void assert_halt() __attribute__((unused));
void assert_halt() {
    digitalWriteFast(PIN_HALT, LOW);
}

void negate_halt() {
    digitalWriteFast(PIN_HALT, HIGH);
}

void negate_tsc() {
    digitalWriteFast(PIN_TSC, LOW);
}

void assert_reset() {
    // Drive RESET condition
    digitalWriteFast(PIN_RESET, LOW);
    negate_halt();
    negate_nmi();
    negate_irq();
    negate_tsc();
    assert_dbe();
}

void negate_reset() {
    // Release RESET conditions
    digitalWriteFast(PIN_RESET, HIGH);
}

constexpr uint8_t PINS_LOW[] = {
        PIN_PHI1,
        PIN_PHI2,
        PIN_RESET,
        PIN_TSC,
};
constexpr uint8_t PINS_HIGH[] = {
        PIN_HALT,
        PIN_IRQ,
        PIN_NMI,
        PIN_DBE,
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
        PIN_VMA,
        PIN_BA,
};

}  // namespace

void PinsMc6800::reset() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);

    assert_reset();
    // Assuming a minimum of 8 clock cycles have occurred.
    for (auto i = 0; i < 8; ++i)
        cycle();
    Signals::resetCycles();
    cycle();
    delayNanoseconds(tpcs_ns);
    negate_reset();
    cycle();
    // Read Reset vector
    cycle();
    cycle();
    // The first instruction will be saving registers, and certainly can be
    // injected.
    _regs->reset();
    _regs->save();
    _regs->setIp(_mems->raw_read16(InstMc6800::VEC_RESET));
    _regs->checkSoftwareType();
}

Signals *PinsMc6800::cycle() {
    return rawCycle();
}

Signals *PinsMc6800::rawCycle() {
    // PHI1=HIGH
    phi1_hi();
    busMode(D, INPUT);
    delayNanoseconds(dbe_lo_ns);
    assert_dbe();
    delayNanoseconds(phi1_hi_ns);
    auto *signals = Signals::put();
    signals->getAddr();
    // PHI2=HIGH
    phi2_hi();
    signals->getDirection();

    if (!signals->valid()) {
        delayNanoseconds(phi2_hi_novma);
    } else if (signals->write()) {
        ++_writes;
        if (signals->writeMemory()) {
            delayNanoseconds(phi2_hi_write);
            signals->getData();
            _mems->write(signals->addr, signals->data);
        } else {
            delayNanoseconds(phi2_hi_capture);
            signals->getData();
        }
    } else {
        _writes = 0;
        if (signals->readMemory()) {
            signals->data = _mems->read(signals->addr);
        } else {
            // inject data from signals->data
            delayNanoseconds(phi2_hi_inject);
        }
        busMode(D, OUTPUT);
        busWrite(D, signals->data);
        delayNanoseconds(phi2_hi_read);
    }
    Signals::nextCycle();
    // PHI2=LOW
    phi2_lo();

    return signals;
}

Signals *PinsMc6800::injectCycle(uint8_t data) {
    Signals::put()->inject(data);
    return cycle();
}

void PinsMc6800::injectReads(const uint8_t *inst, uint8_t len, uint8_t cycles) {
    for (auto inj = 0; inj < len; ++inj) {
        injectCycle(inst[inj]);
    }
    for (auto inj = len; inj < cycles; ++inj) {
        injectCycle(InstMc6800::NOP);
    }
}

void PinsMc6800::captureWrites(uint8_t *buf, uint8_t len, uint16_t *addr) {
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

void PinsMc6800::idle() {
    auto s = Signals::put();
    s->inject(InstMc6800::BRA);
    rawCycle();
    injectCycle(InstMc6800::BRA_HERE);
    injectCycle(InstMc6800::NOP);
    injectCycle(InstMc6800::NOP);
    Signals::discard(s);
}

void PinsMc6800::loop() {
    while (true) {
        _devs->loop();
        rawCycle();
        if (_writes == _regs->contextLength()) {
            const auto frame = Signals::put()->prev(_writes);
            if (nonVmaAfteContextSave())
                cycle();                  // non VMA cycle
            const auto vec_hi = cycle();  // read interruput high(vector)
            const auto vec_swi = _inst->vec_swi();
            if (vec_hi->addr == vec_swi) {
                cycle();  // read interrupt low(vector)
                const auto pc = _regs->capture(frame, false);
                const auto swi_vector = _mems->raw_read16(vec_swi);
                if (isBreakPoint(pc) || swi_vector == vec_swi) {
                    const auto discard = nonVmaAfteContextSave() ? 1 : 2;
                    Signals::discard(frame->prev(discard));
                    return;
                }
            }
        } else if (haltSwitch()) {
            suspend(true);
            return;
        }
    }
}

void PinsMc6800::suspend(bool show) {
    assertNmi();
reentry:
    _writes = 0;
    // Wait for consequtive writes which means registers saved onto stack.
    while (_writes < _regs->contextLength())
        cycle();
    negateNmi();
    // Capture registers pushed onto stack.
    const auto frame = Signals::put()->prev(_writes);
    if (nonVmaAfteContextSave())
        cycle();
    const auto v = cycle();  // hi(vector)
    if (v->addr == _inst->vec_swi()) {
        assertNmi();
        cycle();  // SWI lo(vector);
        goto reentry;
    }
    cycle();  // NMI lo(vector)
    _regs->capture(frame);
    if (show) {
        const auto discard = nonVmaAfteContextSave() ? 1 : 2;
        Signals::discard(frame->prev(discard));
    }
}

void PinsMc6800::run() {
    _regs->restore();
    Signals::resetCycles();
    saveBreakInsts();
    loop();
    restoreBreakInsts();
    disassembleCycles();
}

bool PinsMc6800::step(bool show) {
    Signals::resetCycles();
    _regs->restore();
    if (show)
        Signals::resetCycles();
    suspend(show);
    if (show)
        printCycles(Signals::put()->prev());
    return true;
}

void PinsMc6800::assertNmi() const {
    assert_nmi();
}

void PinsMc6800::negateNmi() const {
    negate_nmi();
}

void PinsMc6800::assertInt(uint8_t name) {
    (void)name;
    assert_irq();
}

void PinsMc6800::negateInt(uint8_t name) {
    (void)name;
    negate_irq();
}

void PinsMc6800::setBreakInst(uint32_t addr) const {
    _mems->put_inst(addr, InstMc6800::SWI);
};

void PinsMc6800::printCycles(const Signals *end) {
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

bool PinsMc6800::matchAll(Signals *begin, const Signals *end) {
    const auto cycles = begin->diff(end->prev());
    LOG_MATCH(cli.print("@@  matchAll: begin="));
    LOG_MATCH(begin->print());
    LOG_MATCH(cli.print("@@           cycles="));
    LOG_MATCH(cli.printlnDec(cycles));
    Signals *prefetch = nullptr;
    for (auto i = 0; i < cycles;) {
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

const Signals *PinsMc6800::findFetch(Signals *begin, const Signals *end) {
    const auto cycles = begin->diff(end);
    // maximum instruction clock is 13.
    const auto limit = cycles < 13 ? cycles : 13;
    LOG_MATCH(cli.print("@@ findFetch: begin="));
    LOG_MATCH(begin->print());
    LOG_MATCH(cli.print("@@              end="));
    LOG_MATCH(end->print());
    for (auto i = 0; i < limit; ++i) {
        for (auto j = i; j < cycles; ++j)
            begin->next(j)->clearFetch();
        auto s = begin->next(i);
        if (matchAll(s, end))
            return s;
        idle();
    }
    for (auto j = 0; j < cycles; ++j)
        begin->next(j)->clearFetch();
    return end;
}

void PinsMc6800::disassembleCycles() {
    const auto end = Signals::put();
    // Make room for idle cycles.
    const auto begin = findFetch(Signals::get()->next(4), end);
    printCycles(begin);
    const auto cycles = begin->diff(end);
    const Signals *pref = nullptr;
    for (auto i = 0; i < cycles;) {
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
}

}  // namespace mc6800
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
