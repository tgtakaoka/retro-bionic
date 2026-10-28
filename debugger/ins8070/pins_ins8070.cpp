#include "pins_ins8070.h"
#include "debugger.h"
#include "devs_ins8070.h"
#include "mems_ins8070.h"
#include "regs_ins8070.h"
#include "signals_ins8070.h"

namespace debugger {
namespace ins8070 {

// clang-format off
/**
 * INS8070 External Bus cycle
 *       __    __    __    __    __    __    __    __    __    __
 *  XOUT   |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__
 *         __    __    __    __    __    __    __    __    __    __
 *   XIN _|  |__| 1|__| 2|__| 3|__| 4|__| 5|__| 6|__| 7|__| 8|__|  |
 *       ________\        \     \     \                 \     \ ____
 * #BREQ         |_____________________________________________|
 *                          _________________________________
 *  ADDR-------------------<_________________________________>------
 *       ________________________                         __________
 *  #RDS                         |______________________r|r
 *       _____________________________                    __________
 *  #WDS                              |wwwwwwwwwwwwwwwwww|
 *
 */
// clang-format on

namespace {

//   fx: min 1.0 MHz, max 4.0 MHz; XIN frequency
//   tx: 1/fx
//  tDS: min 400 ns ; data setup to #RDS+
//  tWS: min 2tx-200 ns ; data valid to #WDS-
// tp1(HOLD): max 2tx read, tx write ; #RDS/#WDS- to #HOLD-
//  ts(HOLD): min 2tx  ; #HOLD- setup
// tp2(HOLD): max 3tx  ; #HOLD+ to #RDS/#WDS+

// XIN runs free from the PWM. #HOLD, low but to end a cycle, keeps the
// address, data and strobe of an external read or write until the cycle
// is served; it is raised then and lowered again once the strobe ends.
// A CPU kept off the bus by #ENIN, or caught in a cycle by #HOLD, waits
// there for the next prepareCycle().

constexpr uint32_t xin_hz = 4054054;  // 150 MHz / 37: halves of 120/127 ns
constexpr uint32_t xin_min_hz = 1000000;
#ifdef TUNE_CLOCK
// K may overclock XIN past fx, to 6.25 MHz.
constexpr auto xin_half_min_ns = 80;
#else
// TW0 and TW1 are 120 ns.
constexpr auto xin_half_min_ns = 120;
#endif
// Teensy cycles in an XIN clock at the slowest rate, so the waits below
// hold at any rate.
constexpr uint32_t xin_max_cycles = F_CPU / xin_min_hz;
// #RST must remain low for 8 Tc.
constexpr auto reset_ns = 4 * 8 * 2 * (1000000000 / xin_min_hz);
// Far past the longest instruction.
constexpr uint32_t strobe_cycles = 1000000 * xin_max_cycles;
// Longer than any wait within a cycle: no edge means no clock or no chip.
constexpr auto edge_cycles = 16 * xin_max_cycles;

void assert_sa() {
    digitalWriteFast(PIN_SA, LOW);
}

void negate_sa() {
    digitalWriteFast(PIN_SA, HIGH);
}

void assert_sb() __attribute__((unused));
void assert_sb() {
    digitalWriteFast(PIN_SB, LOW);
}

void assert_enin() {
    digitalWriteFast(PIN_ENIN, LOW);
}

void negate_enin() {
    digitalWriteFast(PIN_ENIN, HIGH);
}

inline void assert_hold() {
    digitalWriteFast(PIN_HOLD, LOW);
}

inline void negate_hold() {
    digitalWriteFast(PIN_HOLD, HIGH);
}

void negate_reset() {
    digitalWriteFast(PIN_RST, HIGH);
}

constexpr uint8_t PINS_LOW[] = {
        PIN_XIN,
        PIN_RST,
};

constexpr uint8_t PINS_HIGH[] = {
        PIN_SA,
        PIN_SB,
        PIN_ENIN,
        PIN_HOLD,
};

constexpr uint8_t PINS_PULLUP[] = {
        PIN_BREQ,
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
        PIN_RDS,
        PIN_WDS,
        PIN_ENOUT,
        PIN_F1,
        PIN_F2,
        PIN_F3,
};

// Waits up to |limit| Teensy cycles for PIN to read |level|.
template <uint8_t PIN>
inline bool wait_level(uint8_t level, uint32_t limit) {
    const uint32_t start = ARM_DWT_CYCCNT;
    while (digitalReadFast(PIN) != level) {
        if (ARM_DWT_CYCCNT - start >= limit)
            return false;
    }
    return true;
}

}  // namespace

PinsIns8070::PinsIns8070() : _clock(xin_half_min_ns, xin_min_hz) {
    auto regs = new RegsIns8070(this);
    _regs = regs;
    _devs = new DevsIns8070();
    _mems = new MemsIns8070(regs, _devs);
#ifdef TUNE_CLOCK
    _clockInput = xin_hz;
#endif
}

#ifdef TUNE_CLOCK
bool PinsIns8070::setClockInput(uint32_t hz) {
    // The rate reloads at a period start, so the running clock never
    // glitches.
    const auto rate = _clock.setRate(hz);
    if (rate == 0)
        return false;
    _clockInput = rate;
    devs<DevsIns8070>()->sciClock(rate);
    return true;
}
#endif

void PinsIns8070::resetPins() {
    // Assert reset condition
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_PULLUP, sizeof(PINS_PULLUP), INPUT_PULLUP);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
#ifdef TUNE_CLOCK
    const auto hz = _clockInput;
#else
    const auto hz = xin_hz;
#endif
    _clock.begin(hz);
    _clock.start();
    devs<DevsIns8070>()->sciClock(hz);

    delayNanoseconds(reset_ns);
    assert_hold();
    negate_reset();
    Cycles::reset();
    // The first fetch comes within 13 Tc after #RST rises; the save's
    // first prepareCycle() waits for it.
    _regs->save();
}

Signals *PinsIns8070::prepareCycle() {
    auto s = Signals::put();
    // Once a cycle at least: one held by #HOLD skips the wait below.
    devs<DevsIns8070>()->sciLoop();
    const uint32_t start = ARM_DWT_CYCCNT;
    while (!s->getDirection()) {
        devs<DevsIns8070>()->sciLoop();
        if (ARM_DWT_CYCCNT - start >= strobe_cycles) {
            // A CPU that starts no bus cycle; give up rather than wedge
            // the board, leaving the halt switch to stop the run.
            cli.println("?halt: no bus cycle");
            s->noCycle();
            return s;
        }
    }
    // assert_debug();
    s->getAddr();
    // negate_debug();
    return s;
}

// Write data is valid with the address, ahead of #WDS. Read data is
// driven before #HOLD rises and released once #RDS does.
Signals *PinsIns8070::completeCycle(Signals *s) {
    if (s->write()) {
        s->getData();
        if (s->writeMemory())
            _mems->write(s->addr, s->data);
        noInterrupts();
        negate_hold();
        wait_level<PIN_WDS>(HIGH, edge_cycles);
        // assert_debug();
    } else if (s->read()) {
        if (s->readMemory())
            s->data = _mems->read(s->addr);
        s->outData();
        noInterrupts();
        negate_hold();
        wait_level<PIN_RDS>(HIGH, edge_cycles);
        // assert_debug();
        Signals::inputMode();
    } else {
        return s;  // no cycle
    }
    assert_hold();
    // negate_debug();
    interrupts();
    Cycles::next();
    return s;
}

Signals *PinsIns8070::cycle() {
    return completeCycle(prepareCycle());
}

Signals *PinsIns8070::inject(uint8_t data) {
    return completeCycle(prepareCycle()->inject(data));
}

void PinsIns8070::execInst(const uint8_t *inst, uint8_t len) {
    execute(inst, len, nullptr, nullptr, 0);
}

void PinsIns8070::captureWrites(const uint8_t *inst, uint8_t len,
        uint16_t *addr, uint8_t *buf, uint8_t max) {
    execute(inst, len, addr, buf, max);
}

void PinsIns8070::execute(const uint8_t *inst, uint8_t len, uint16_t *addr,
        uint8_t *buf, uint8_t max) {
    uint8_t inj = 0;
    uint8_t cap = 0;
    assert_enin();
    while (inj < len || cap < max) {
        auto s = prepareCycle();
        if (inj < len)
            s->inject(inst[inj]);
        if (cap < max)
            s->capture();
        completeCycle(s);
        if (s->read() && inj < len) {
            ++inj;
        } else if (s->write()) {
            if (cap == 0 && addr)
                *addr = s->addr;
            if (buf && cap < max)
                buf[cap++] = s->data;
        }
    }
    negate_enin();
}

// The breakpoint's CALL 15 whose vector |vector| reads: its opcode three
// cycles before, then its two pushes. A program's own CALL 15 counts when
// its vector is 0000H.
const Signals *PinsIns8070::isCall15(const Signals *vector) const {
    if (vector->read()) {
        const auto call = vector->prev(3);
        if (call->read() && call->data == InstIns8070::CALL15) {
            const auto push = call->next();
            if (push->write() && push->next()->write()) {
                const auto vec15 = _mems->read16(InstIns8070::VEC_CALL15);
                if (isBreakPoint(call->addr) || vec15 == 0)
                    return call;
            }
        }
    }
    return nullptr;
}

bool PinsIns8070::loop() {
#ifdef PROFILE_CYCLES
    // For scripts/record-cycles.py: give up well before the ring wraps; the
    // debug pin frames the run, to trigger a capture on.
    constexpr auto MAX_CYCLES = 96;
    assert_debug();
    for (auto n = 0;; ++n) {
#else
    while (true) {
#endif
        _devs->loop();
        auto s = prepareCycle();
        if (s->addr == InstIns8070::VEC_CALL15) {
            const auto call = isCall15(s);
            if (call) {
                // Vector 0000H, then a RET popping the CALL's address one
                // early, as the PC is incremented before a fetch.
                const uint16_t pc = call->addr - 1;
                inject(0);                 // vector low
                inject(0);                 // vector high
                inject(InstIns8070::RET);  // cancel CALL 15
                inject(lo(pc));            // return address low
                inject(hi(pc));            // return address high
                Cycles::discard(call);
#ifdef PROFILE_CYCLES
                negate_debug();
#endif
                return true;
            }
        }
        completeCycle(s);
#ifdef PROFILE_CYCLES
        if (n >= MAX_CYCLES || haltSwitch()) {
            negate_debug();
            cli.println(n >= MAX_CYCLES ? "?cycles" : "?halt");
#else
        if (haltSwitch()) {
#endif
            return suspend();
        }
    }
}

// Bus cycles to wait for the next opcode fetch; an instruction takes far
// fewer.
constexpr auto fetch_cycles = 64;

bool PinsIns8070::suspend() {
    // Bound the wait; loop() polls the halt switch only between steps.
    for (auto n = 0; n < fetch_cycles; ++n) {
        auto s = prepareCycle();
        if (s->fetch()) {
            // A BRA $ refetches this opcode on the next run.
            completeCycle(s->inject(InstIns8070::BRA));
            inject(InstIns8070::BRA_HERE);
            Cycles::discard(s);
            return true;
        }
        completeCycle(s);
    }
    cli.println("?halt: no fetch");
    return false;
}

void PinsIns8070::run() {
    _regs->restore();
    Cycles::reset();
    saveBreakInsts();
    assert_enin();
    startRunTimer();
    const auto stopped = loop();
    stopRunTimer();
    negate_enin();
    restoreBreakInsts();
    disassembleCycles();
    // A failed halt leaves the CPU running: keep the registers from the
    // last good save.
    if (stopped)
        _regs->save();
}

uint8_t PinsIns8070::busCycles(InstIns8070 &inst) const {
    auto pc = _regs->nextIp();
    const auto opc = _mems->read_byte(pc);
    if (!inst.get(opc))
        return 0;
    const auto opr = _mems->read_byte(pc + 1);
    const auto ea = regs<RegsIns8070>()->effectiveAddr(inst, opr);
    return MemsIns8070::is_internal(ea) ? inst.externalCycles()
                                        : inst.busCycles();
}

bool PinsIns8070::step(bool show) {
    Cycles::reset();
    InstIns8070 inst;
    const auto cycles = busCycles(inst);
    if (cycles == 0)
        return false;
    if (inst.opc == InstIns8070::CALL15 &&
            _mems->read16(InstIns8070::VEC_CALL15) == 0)
        return false;
    _regs->restore();
    if (show)
        Cycles::reset();
    assert_enin();
    if (inst.addrMode() == M_SSM) {
        // SSM's reads depend on the search: run to the fetch at SSM + 3
        // (next + 2), bounded at 256 cycles.
        const auto addr = cycle()->addr;
        for (auto n = 0; n < 256; ++n) {
            auto s = prepareCycle();
            if (s->addr == addr + 3) {
                completeCycle(s->inject(InstIns8070::BRA));
                inject(InstIns8070::BRA_HERE);
                if (show)
                    Cycles::discard(s);
                break;
            }
            completeCycle(s);
        }
    } else {
        for (auto c = 0; c < cycles; ++c) {
            cycle();
        }
    }
    if (show)
        printCycles();
    _regs->save();
    return true;
}

void PinsIns8070::assertInt(uint8_t) {
    // #INTA is negative-edge sensed.
    assert_sa();
}

void PinsIns8070::negateInt(uint8_t) {
    negate_sa();
}

void PinsIns8070::setBreakInst(uint32_t addr) const {
    _mems->put_prog(addr, InstIns8070::CALL15);
}

void PinsIns8070::printCycles(const Signals *end) {
    const auto g = Signals::get();
    const auto cycles = g->diff(end ? end : Signals::put());
    for (auto i = 0u; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

namespace {
// The board's: its bus kept alive while the walk goes.
struct BoardArch final : ArchIns8070 {
    BoardArch(const MatchMemory &mems, PinsIns8070 &pins)
        : ArchIns8070(mems), _pins(pins) {}
    void idle() const override { _pins.idle(); }

private:
    PinsIns8070 &_pins;
};
}  // namespace

// The walk takes in the cycle at |end| too: the next fetch, which the
// board leaves there.
const Signals *PinsIns8070::findFetch(Signals *begin, const Signals *end) {
    const BoardArch arch(*_mems, *this);
    auto &walker = MatchWalker::shared();
    if (!walker.walk(arch, begin, end->next()))
        return end;
    return begin->next(walker.start());
}

// Counts fetchMark(), which findFetch() sets by walking the ring, not the
// on-the-fly fetch() that signals.h's backtraceStartFrom() counts.
// printBacktrace() runs findFetch() again and derives the same marks.
const SignalsImpl *PinsIns8070::findBacktraceStart() {
    const auto end = Signals::put();
    const auto begin = findFetch(Signals::get(), end);
    if (_lineLimit == 0)
        return end;
    auto limit = _lineLimit;
    auto s = end;
    while (s != begin) {
        s = s->prev();
        if (s->fetchMark() && --limit == 0)
            return s;
    }
    return begin;
}

void PinsIns8070::printBacktrace() {
    const auto end = Signals::put();
#ifdef PROFILE_CYCLES
    // Every cycle, with the matcher's marks.
    findFetch(Signals::get(), end);
    printCycles(end);
#else
    const auto begin = findFetch(Signals::get(), end);
    printCycles(begin);
    const auto cycles = begin->diff(end);
    if (Debugger.verbose()) {
        // Every cycle in order, an instruction's line before its fetch.
        for (auto i = 0u; i < cycles; ++i) {
            const auto s = begin->next(i);
            if (s->fetchMark())
                _mems->disassemble(s->addr, 1);
            s->print();
            idle();
        }
        return;
    }
    for (auto i = 0u; i < cycles;) {
        const auto s = begin->next(i);
        if (s->fetchMark()) {
            const auto len = _mems->disassemble(s->addr, 1) - s->addr;
            for (uint_fast8_t j = 1; j < len; j++) {
                const auto t = s->next(j);
                if (t->addr < s->addr || t->addr >= s->addr + len)
                    t->print();
            }
            i += len;
        } else {
            s->print();
            ++i;
        }
        idle();
    }
#endif
}

}  // namespace ins8070
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
