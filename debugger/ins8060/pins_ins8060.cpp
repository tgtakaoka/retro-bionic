#include "pins_ins8060.h"
#include "debugger.h"
#include "devs_ins8060.h"
#include "inst_ins8060.h"
#include "mems_ins8060.h"
#include "regs_ins8060.h"
#include "signals_ins8060.h"
#include "watchdog.h"

namespace debugger {
namespace ins8060 {

// clang-format off
/**
 * INS8060 External Bus cycle
 *        __    __    __    __    __    __    __    __    __    __
 *    XIN   |__| 1|__| 2|__| 3|__| 4|__| 5|__| 6|__| 7|__| 8|__|  |
 *          \ __  \ __  \ __  \ __  \ _\  \ __  \ __  \ __  \ __
 *   XOUT |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__
 *        _____                |     |  |  \           \     \  ___
 *  #BREQ      |_______________|_____|__|___|___________|______|
 *        ____                 |     |  |   |           |      ____
 * #ENOUT     |________________|_____|__|___|___________|_____|
 *        __________           |     |  |   |           |
 *  #ENIN           |__________|_____|__|___|___________|_________|
 *        _____________________|     |__|___|___________|__________
 *   #ADS                      |_____|  |   |           |
 *        ______________________________|   |           |__________
 *   #RDS                               |___|___________|
 *        __________________________________|           |__________
 *   #WDS                                   |___________|
 *                          ______________________________
 *    AD ------------------<______________________________>--------
 *                          ___________ __________________
 *    DB ------------------<_____AD____X_________W_____R__>--------
 */
// clang-format on

namespace {
//   fx: min 0.1 MHz, max 4.0 MHz; XIN frequency
//   Tc: min 500 ns     ; 2 cycles of XIN, half a microcycle
//  TW0: min 120 ns     ; XIN low width
//  TW1: min 120 ns     ; XIN high width
// TSAD: min Tc/2-165 ns; address setup to #ADS-
// THAD: min       50 ns; address hold from #RDS/#WDS+
// TSST: min Tc/2-150 ns; status setup to #ADS-
// THST: min       50 ns; status hold from #ADS+
// TSRD: min      175 ns; data setup to #RDS+
// THRD: min        0 ns; data hold from #RDS+
// TSWD: min Tc/2-200 ns; data setup to #WDS-
// THWD:          100 ns; data hold from #WDS+
// Tp2(ADS): Tc/2-3Tc/2 ; #ENIN- to #ADS-
// TS(HOLD): min  200 ns; #HOLD- to the strobe's unextended end
// Tp1(HOLD): 130-275 ns; #HOLD+ to #RDS/#WDS+
// TH(HOLD): min    0 ns; #HOLD hold from #RDS/#WDS+

// XIN runs free from the PWM. #ENIN, high between cycles, grants each
// cycle, with interrupts masked until #ADS has put the status and A12-A15
// on DB. #HOLD, low but to end a cycle, stretches its strobe until the
// cycle is served. #ENIN goes high again only once the cycle is over, as
// one it cuts short is repeated; an ILD or DLD keeps #BREQ low and waits
// for it to store.

constexpr uint32_t xin_hz = 4054054;  // 150 MHz / 37: halves of 120/127 ns
constexpr uint32_t xin_min_hz = 100000;
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
// #RST must remain low for a minimum of 4 Tc.
constexpr auto reset_ns = 2 * 4 * (1000000000 / xin_min_hz);
// DLY, the longest instruction, keeps #BREQ high for up to 131606
// microcycles of 4 XIN.
constexpr uint32_t breq_cycles = 131606u * 4 * xin_max_cycles;
static_assert(uint64_t(131606u) * 4 * xin_max_cycles < (uint64_t(1) << 32),
        "a cycle counter span");
// Longer than any wait within a cycle: no edge means no clock or no chip.
constexpr auto edge_cycles = 16 * xin_max_cycles;
// XIN clocks #ENIN is granted ahead for, past the gap between most
// cycles.
constexpr auto grant_xins = 48;
// XIN clocks from the strobe's end to #BREQ released.
constexpr auto release_xins = 4;
// Read data is driven before #HOLD rises: TSRD less Tp1(HOLD).
constexpr auto read_setup_ns = 175 - 130;

inline auto signal_breq() {
    return digitalReadFast(PIN_BREQ);
}

void assert_sense_a() {
    digitalWriteFast(PIN_SENSEA, HIGH);
}

void negate_sense_a() {
    digitalWriteFast(PIN_SENSEA, LOW);
}

inline void assert_enin() {
    digitalWriteFast(PIN_ENIN, LOW);
}

inline void negate_enin() {
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
        PIN_SENSEA,
        PIN_SENSEB,
        PIN_SIN,
};

constexpr uint8_t PINS_HIGH[] = {
        PIN_CONT,
        PIN_ENIN,
        PIN_HOLD,
};

constexpr uint8_t PINS_PULLUP[] = {
        PIN_BREQ,
};

constexpr uint8_t PINS_INPUT[] = {
        PIN_DB0,
        PIN_DB1,
        PIN_DB2,
        PIN_DB3,
        PIN_DB4,
        PIN_DB5,
        PIN_DB6,
        PIN_DB7,
        PIN_ADL00,
        PIN_ADL01,
        PIN_ADL02,
        PIN_ADL03,
        PIN_ADL04,
        PIN_ADL05,
        PIN_ADL06,
        PIN_ADL07,
        PIN_ADM08,
        PIN_ADM09,
        PIN_ADM10,
        PIN_ADM11,
        PIN_FLAG0,
        PIN_FLAG1,
        PIN_FLAG2,
        PIN_RDS,
        PIN_WDS,
        PIN_ADS,
        PIN_ENOUT,
        PIN_SOUT,
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

// Gives up a wait rather than wedge the board, leaving the halt switch to
// stop the run.
void no_bus_cycle() {
    cli.println("?halt: no bus cycle");
}

}  // namespace

PinsIns8060::PinsIns8060() : _clock(xin_half_min_ns, xin_min_hz) {
    _regs = new RegsIns8060(this);
    _devs = new DevsIns8060();
    _mems = new MemsIns8060(_devs);
#ifdef TUNE_CLOCK
    _clockInput = xin_hz;
#endif
}

#ifdef TUNE_CLOCK
bool PinsIns8060::setClockInput(uint32_t hz) {
    // The rate reloads at a period start, so the running clock never
    // glitches.
    const auto rate = _clock.setRate(hz);
    if (rate == 0)
        return false;
    _clockInput = rate;
    devs<DevsIns8060>()->sciClock(rate);
    return true;
}
#endif

void PinsIns8060::resetPins() {
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
    devs<DevsIns8060>()->sciClock(hz);

    Cycles::reset();
    delayNanoseconds(reset_ns);
    assert_hold();
    negate_reset();
    // #BREQ goes low within 13 Tc after #RST rises; the save's first
    // prepareCycle() waits for its bus cycle.
    _regs->save();
}

Signals *PinsIns8060::prepareCycle() const {
    auto s = Signals::put();
    // Once a cycle at least: a granted cycle often skips the wait below.
    devs<DevsIns8060>()->sciLoop();
    // The status and A12-A15 are on DB only until just after #ADS rises.
    // #ENIN low before #BREQ falls lets #ADS follow at once; asserted
    // after, it is synchronised first, a Tc more at some rates.
    noInterrupts();
    assert_enin();
    if (wait_level<PIN_ADS>(LOW, grant_xins * xinCycles())) {
        s->getAddr();
        interrupts();
        return s;
    }
    // A cycle #ENIN cuts short is repeated once it is low again.
    negate_enin();
    interrupts();
    // A DLY keeps #BREQ high for as long as 50 ms.
    const uint32_t start = ARM_DWT_CYCCNT;
    while (signal_breq() != LOW) {
        Watchdog::tick();
        devs<DevsIns8060>()->sciLoop();
        if (ARM_DWT_CYCCNT - start >= breq_cycles) {
            no_bus_cycle();
            s->noCycle();
            return s;
        }
    }
    noInterrupts();
    assert_enin();
    if (!wait_level<PIN_ADS>(LOW, edge_cycles)) {
        negate_enin();
        interrupts();
        no_bus_cycle();
        s->noCycle();
        return s;
    }
    // assert_debug();
    s->getAddr();
    // negate_debug();
    interrupts();
    return s;
}

// Write data is valid by #WDS falling. Read data is driven once #RDS
// falls, as DB carries the status until then, and released once it rises.
Signals *PinsIns8060::completeCycle(Signals *s) const {
    const auto read = s->read();
    if (read) {
        if (s->readMemory())
            s->data = _mems->read(s->addr);
        if (!wait_level<PIN_RDS>(LOW, edge_cycles))
            return s;  // no cycle
        s->outData();
        noInterrupts();
        delayNanoseconds(read_setup_ns);
        negate_hold();
        wait_level<PIN_RDS>(HIGH, edge_cycles);
        // assert_debug();
        Signals::inputMode();
    } else {
        if (!wait_level<PIN_WDS>(LOW, edge_cycles))
            return s;  // no cycle
        // assert_debug();
        s->getData();
        if (s->writeMemory())
            _mems->write(s->addr, s->data);
        noInterrupts();
        negate_hold();
        wait_level<PIN_WDS>(HIGH, edge_cycles);
        // assert_debug();
    }
    // The cycle is over once the CPU lets #BREQ go and #ENOUT follows
    // #ENIN low; an ILD or DLD holds it to store.
    wait_level<PIN_ENOUT>(LOW, release_xins * xinCycles());
    negate_enin();
    assert_hold();
    // negate_debug();
    interrupts();
    Cycles::next();
    return s;
}

uint32_t PinsIns8060::xinCycles() const {
#ifdef TUNE_CLOCK
    return F_CPU / _clockInput;
#else
    return F_CPU / xin_hz;
#endif
}

Signals *PinsIns8060::inject(uint8_t data) const {
    const auto s = prepareCycle()->inject(data);
    return completeCycle(s);
}

void PinsIns8060::execInst(const uint8_t *inst, uint8_t len) const {
    execute(inst, len, nullptr, nullptr, 0);
}

void PinsIns8060::captureWrites(const uint8_t *inst, uint8_t len,
        uint16_t *addr, uint8_t *buf, uint8_t max) const {
    execute(inst, len, addr, buf, max);
}

void PinsIns8060::execute(const uint8_t *inst, uint8_t len, uint16_t *addr,
        uint8_t *buf, uint8_t max) const {
    uint8_t inj = 0;
    uint8_t cap = 0;
    while (inj < len || cap < max) {
        auto s = prepareCycle();
        if (inj < len)
            s->inject(inst[inj]);
        if (cap < max)
            s->capture();
        completeCycle(s);
        if (s->read()) {
            if (inj < len)
                ++inj;
        } else if (s->write()) {
            if (cap == 0 && addr)
                *addr = s->addr;
            if (buf && cap < max)
                buf[cap++] = s->data;
        }
    }
    suspend();
}

bool PinsIns8060::loop() {
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
        completeCycle(s);
        if (s->halt()) {
#ifdef PROFILE_CYCLES
            negate_debug();
#endif
            // Park on a jump back to the HALT, kept off the bus by #ENIN.
            inject(InstIns8060::JMP);
            inject(InstIns8060::JMP_HALT);
#ifdef PROFILE_CYCLES
            // Keep the HALT's second read, the one with the H flag.
            Cycles::discard(s->next());
#else
            Cycles::discard(s);
#endif
            return true;
        }
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

bool PinsIns8060::suspend() const {
    // Bound the wait; loop() polls the halt switch only between steps.
    for (auto n = 0; n < fetch_cycles; ++n) {
        auto s = prepareCycle();
        if (s->fetch()) {
            // A JMP $ refetches this opcode once #ENIN lets the CPU go.
            completeCycle(s->inject(InstIns8060::JMP));
            inject(InstIns8060::JMP_HERE);
            Cycles::discard(s);
            return true;
        }
        completeCycle(s);
    }
    cli.println("?halt: no fetch");
    return false;
}

void PinsIns8060::run() {
    _regs->restore();
    Cycles::reset();
    saveBreakInsts();
    startRunTimer();
    const auto stopped = loop();
    stopRunTimer();
    restoreBreakInsts();
    disassembleCycles();
    // A failed halt leaves the CPU running: keep the registers from the
    // last good save.
    if (stopped)
        _regs->save();
}

bool PinsIns8060::rawStep() const {
    auto s = prepareCycle();
    completeCycle(s);
    if (s->halt()) {
        inject(InstIns8060::JMP);
        inject(InstIns8060::JMP_HALT);
        Cycles::discard(s);
        return false;
    }
    return suspend();
}

bool PinsIns8060::step(bool show) {
    Cycles::reset();
    _regs->restore();
    if (show)
        Cycles::reset();
    if (rawStep()) {
        if (show)
            printCycles();
        _regs->save();
        return true;
    }
    return false;
}

void PinsIns8060::assertInt(uint8_t) {
    assert_sense_a();
}

void PinsIns8060::negateInt(uint8_t) {
    negate_sense_a();
}

void PinsIns8060::setBreakInst(uint32_t addr) const {
    _mems->put_prog(addr, InstIns8060::HALT);
}

void PinsIns8060::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

const SignalsImpl *PinsIns8060::findBacktraceStart() {
    return backtraceStartByFetchCount<Signals>(_lineLimit);
}

void PinsIns8060::printBacktrace() {
#ifdef PROFILE_CYCLES
    // Every cycle, with the chip's status flags.
    printCycles();
#else
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles;) {
        const auto s = g->next(i);
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

}  // namespace ins8060
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
