#include "pins_i8080.h"
#include "debugger.h"
#include "devs_i8080.h"
#include "inst_i8080.h"
#include "mems_i8080.h"
#include "regs_i8080.h"
#include "signals_i8080.h"

// #define DEBUG(e) e
#define DEBUG(e)

namespace debugger {
namespace i8080 {

// clang-format off
/**
 * P8080 bus cycle.
 *       --|---T1---|---T2---|---T3---|---T4---|---T5---|---T1---|
 *          _        _        _        _        _        _
 * phi 1 __/ \______/ \______/ \______/ \______/ \______/ \______
 *             ___      ___      ___      ___      ___      ___
 * phi 2 _____/   \____/   \____/   \____/   \____/   \____/   \_
 *             ___v____
 *  SYNC _____/        \_________________________________________
 *                       _______
 *  DBIN _______________/       \________________________________
 *       ____________________          __________________________
 *   #WR                     \________/
 *       _________v______________________________________________
 * READY ________/ \_____________________________________________
 */
// clang-format on

// The period in counts of the 150 MHz bus clock. PHI1 and PHI2 reach the
// CPU through passive level shifters: a fall is quick, but a rise waits
// on the 12 V pull-up, so a phase there is narrower than here and its gap
// wider. The widths are fixed, and only tD2 follows the rate.
struct Phases {
    uint16_t n;       // tCY
    uint16_t phi1;    // PHI1 high from 0
    uint16_t on2;     // PHI2 high from here
    uint16_t off2;    // to here
    // The status is valid tDD (220 ns) after PHI2 rises at the CPU, up
    // to 60 ns after it rises here; read earlier, D0-D7 still hold the
    // last cycle's data, which may pass for a write's status.
    uint16_t status;
    // READY is sampled as PHI2 falls, 50 ns later at the CPU, with tRS
    // 120 ns: it changes outside these counts.
    uint16_t holdFrom;
    uint16_t holdTo;

    // A shorter period takes tD2 down to 8 counts first, then PHI2 down
    // to 30.
    static constexpr uint16_t phi2Of(uint16_t counts) {
        return counts >= 15 + 3 + 42 + 8   ? 42
               : counts >= 15 + 3 + 30 + 8 ? counts - (15 + 3 + 8)
                                           : 30;
    }

    constexpr Phases(uint16_t counts)
        : n(counts),
          phi1(15),     // 100 ns
          on2(15 + 3),  // 20 ns
          off2(15 + 3 + phi2Of(counts)),
          status(15 + 3 + (320 * 150 + 999) / 1000),
          holdFrom(15 + 3 + phi2Of(counts) - 12),
          holdTo(15 + 3 + phi2Of(counts) + 12) {}
    uint32_t hz() const { return F_BUS_ACTUAL / n; }
};
namespace {
// [i8224]
//  tCYC: min 500 ns; PHI1/PHI2 cycle time
// tPHI1: min  89 ns; PHI1 high width 2/9 tCYC-23ns
//   tD1: min   0 ns; PHI1- to PHI2+ delay
// tPHI2: min 236 ns; PHI2 high width 5/9 tCYC-35ns
//   tD2: min  95 ns; PHI2- to PHI1+ delay
//   tD3: min 109 ns; PHI1+ to PHI2+ delay
// [i8080]
//   tDC: max 150 ns; PHI2+ to SYNC+ and SYNC-
//   tDF: max 140 ns; PHI2+ to DBIN+ and DBIN-
//   tDC: max 150 ns; PHI1+ to #WR- and #WR+
//   tDA: max 200 ns; PHI2+ to address valid
//   tDD: max 220 ns; PHI2+ to status valid
//  tDS1: min  30 ns; data in setup to PHI1+
//  tDS2: min 150 ns; data in setup to PHI2+
//   tDH: min  50 ns; data in hold from PHI2+
//   tDD: max 220 ns; PHI2+ to data out
//   tRS: min 120 ns; READY setup to PHI2-
//    tH: min   0 ns; READY hold from PHI2-
// PHI1 and PHI2 run free from two FlexPWM submodules, their counters
// restarted together, and the CPU waits in each cycle with
// READY low. SYNC starts a cycle, and READY goes low as it rises; the
// status on D0-D7 is read once valid, before PHI2 of T2. READY goes high
// once the cycle is served, and stays high until the next SYNC.

// 150 MHz / 60: tCY 400 ns, PHI2 34 counts, about 145 ns at the CPU.
constexpr Phases phi_phases{60};
// tCY max 2,000 ns.
constexpr uint32_t phi_min_hz = 500000;
#ifdef TUNE_CLOCK
// K may narrow the phases past the CPU's limits, to find where it stops
// keeping up.
constexpr uint16_t phi_min_counts = 52;
#else
constexpr uint16_t phi_min_counts = 60;
#endif
// PHI2's counter starts this many counts after PHI1's, the two restart
// writes apart; measured in a capture.
constexpr uint16_t phi2_lag = 4;
// Teensy cycles in a clock period at the slowest rate, so the waits
// below hold at any rate.
constexpr uint32_t phi_max_cycles = F_CPU / phi_min_hz;
// An external RESET signal of three clock period duration (minimum).
constexpr auto reset_ns = 10 * (1000000000 / phi_min_hz);
// The gap between SYNCs is a few states; a halted CPU makes none.
constexpr auto sync_cycles = 16 * phi_max_cycles;
// DBIN falls a state after READY is sampled high.
constexpr auto dbin_cycles = 4 * phi_max_cycles;
// Clock periods from T2 to the TW a cycle waits in.
constexpr auto wait_periods = 3;
// A run holds a fetch this often to run the devices and poll the halt
// switch, whose yield() takes longer than a cycle; in between, plain
// memory reads take no wait state.
constexpr uint32_t service_cycles = F_CPU / 100000;

using Phi1 = PwmClock<PIN_PHI1>;
using Phi2 = PwmClock<PIN_PHI2>;

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

// Waits up to |limit| Teensy cycles for DBIN or WAIT to go high.
inline bool wait_dbin_or_wait(uint32_t limit) {
    const uint32_t start = ARM_DWT_CYCCNT;
    while (digitalReadFast(PIN_DBIN) == LOW && digitalReadFast(PIN_WAIT) == LOW) {
        if (ARM_DWT_CYCCNT - start >= limit)
            return false;
    }
    return true;
}

// Waits up to a period for the status to be valid: from PHI1's count
// |status|, which may fall in the next period, to PHI2's rise of T2.
inline void wait_status(const Phases &p, uint32_t limit) {
    const uint32_t start = ARM_DWT_CYCCNT;
    while (true) {
        const uint16_t c = Phi1::count();
        if (p.status < p.n ? (c >= p.status || c < p.on2)
                           : (c >= p.status - p.n && c < p.on2))
            return;
        if (ARM_DWT_CYCCNT - start >= limit)
            return;
    }
}

void assert_ready() {
    digitalWriteFast(PIN_READY, HIGH);
}

// Whether PHI1's count |c| is where the CPU samples READY: a change there
// misses tRS, and the CPU may wait in part.
bool in_hold(const Phases &p, uint16_t c) {
    return p.holdTo <= p.n ? (c >= p.holdFrom && c < p.holdTo)
                           : (c >= p.holdFrom || c < p.holdTo - p.n);
}

// Raises READY outside the counts where the CPU samples it.
void raise_ready(const Phases &p) {
    const uint32_t start = ARM_DWT_CYCCNT;
    while (in_hold(p, Phi1::count())) {
        if (ARM_DWT_CYCCNT - start >= phi_max_cycles)
            break;
    }
    assert_ready();
}

void negate_ready() {
    digitalWriteFast(PIN_READY, LOW);
}

void assert_reset() {
    digitalWriteFast(PIN_RESET, HIGH);
}

void negate_reset() {
    digitalWriteFast(PIN_RESET, LOW);
}

const uint8_t PINS_LOW[] = {
        PIN_PHI1,
        PIN_PHI2,
        PIN_INT,
        PIN_HOLD,
        PIN_READY,
};

const uint8_t PINS_HIGH[] = {
        PIN_RESET,
};

const uint8_t PINS_INPUT[] = {
        PIN_D0,
        PIN_D1,
        PIN_D2,
        PIN_D3,
        PIN_D4,
        PIN_D5,
        PIN_D6,
        PIN_D7,
        PIN_ADDR0,
        PIN_ADDR1,
        PIN_ADDR2,
        PIN_ADDR3,
        PIN_ADDR4,
        PIN_ADDR5,
        PIN_ADDR6,
        PIN_ADDR7,
        PIN_ADDR8,
        PIN_ADDR9,
        PIN_ADDR10,
        PIN_ADDR11,
        PIN_ADDR12,
        PIN_ADDR13,
        PIN_ADDR14,
        PIN_ADDR15,
        PIN_DBIN,
        PIN_WR,
        PIN_SYNC,
        PIN_HLDA,
        PIN_INTE,
        PIN_WAIT,
};

}  // namespace

PinsI8080::PinsI8080() : _phi1(0, phi_min_hz), _phi2(0, phi_min_hz) {
    _regs = new RegsI8080(this);
    _mems = new MemsI8080();
    _devs = new DevsI8080();
#ifdef TUNE_CLOCK
    _counts = phi_phases.n;
#endif
}

const Phases &PinsI8080::phases() const {
#ifdef TUNE_CLOCK
    static Phases p{phi_phases};
    if (p.n != _counts)
        p = Phases(_counts);
    return p;
#else
    return phi_phases;
#endif
}

uint32_t PinsI8080::phiCycles() const {
    return F_CPU / phases().hz();
}

// Both phases at the rate, and the two counters restarted together once
// the rate has loaded, at the end of the period under way: at boot the
// core's default, about 223 us.
void PinsI8080::setPhases() const {
    const auto &p = phases();
    _phi1.setRate(p.hz());
    _phi2.setRate(p.hz());
    Phi1::setEdges(0, p.phi1);
    Phi2::setEdges(p.on2 - phi2_lag, p.off2 - phi2_lag);
    delayMicroseconds(300);
    const auto now1 = Phi1::restartNow();
    const auto now2 = Phi2::restartNow();
    const auto done1 = Phi1::restartDone();
    const auto done2 = Phi2::restartDone();
    noInterrupts();
    Phi1::sm().CTRL2 = now1;
    Phi2::sm().CTRL2 = now2;
    interrupts();
    Phi1::sm().CTRL2 = done1;
    Phi2::sm().CTRL2 = done2;
}

#ifdef TUNE_CLOCK
bool PinsI8080::setClockInput(uint32_t hz) {
    if (hz < phi_min_hz)
        return false;
    const uint16_t counts = (F_BUS_ACTUAL + hz / 2) / hz;
    if (counts < phi_min_counts)
        return false;
    _counts = counts;
    setPhases();
    return true;
}

uint32_t PinsI8080::clockInput() const {
    return phases().hz();
}
#endif

void PinsI8080::resetPins() {
    // Assert reset condition
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
    const auto hz = phases().hz();
    _phi1.begin(hz);
    _phi2.begin(hz);
    setPhases();
    _phi1.start();
    _phi2.start();
    Cycles::reset();

    delayNanoseconds(reset_ns);
    negate_reset();
    noInterrupts();
    latchCycle();
    interrupts();
    _regs->save();
}

// Waits for the next SYNC, lowers READY for its cycle and latches its
// address and status into Signals::put(), once the cycle waits on its
// strobe: DBIN for a read, WAIT for the others. The status is on D0-D7
// only until PHI2 of T2, so the caller masks interrupts. In a run, given
// when it was last |serviced|, a plain memory read is served here with
// READY raised again before T2 samples it, so it takes no wait state, and
// the next cycle latched, until one needs the debugger. Returns false
// when no SYNC comes, as after a HLT.
bool PinsI8080::latchCycle(const uint32_t *serviced) const {
    const auto &p = phases();
    const auto period = phiCycles();
    while (true) {
        auto s = Signals::put();
        if (!wait_level<PIN_SYNC>(HIGH, sync_cycles))
            return false;
        negate_ready();
        wait_status(p, period);
        // assert_debug();
        s->getStatus();
        s->getAddress();
        // negate_debug();
        if (s->halt())
            return true;
        if (!s->read()) {
            wait_level<PIN_WAIT>(HIGH, wait_periods * period);
            return true;
        }
        const auto running = serviced != nullptr;
        const auto serviceDue =
                running && ARM_DWT_CYCCNT - *serviced >= service_cycles;
        // A fetch is held when the devices and halt switch are due.
        const auto heldFetch = s->fetch() && serviceDue;
        const auto zeroWait = running && s->memory() && !heldFetch;
        // READY goes high again before reading memory: wait_status() has
        // left the count in late T1 or early T2, clear of where T2 samples
        // it.
        if (zeroWait)
            assert_ready();
        if (s->memory())
            s->data = _mems->read(s->addr);
        if (!zeroWait) {
            // A bus idle cycle has a read status but no DBIN.
            wait_dbin_or_wait(wait_periods * period);
            return true;
        }
        // The data is due for T3, about 85 ns after DBIN rises at 2.5 MHz.
        s->stageData();
        if (wait_level<PIN_DBIN>(HIGH, wait_periods * period)) {
            Signals::driveData();
            wait_level<PIN_DBIN>(LOW, dbin_cycles);
            Signals::inputMode();
        }
        Cycles::next();
    }
}

Signals *PinsI8080::prepareCycle() const {
    return Signals::put();
}

Signals *PinsI8080::resumeCycle() const {
    return Signals::put();
}

Signals *PinsI8080::resumeCycle(uint16_t pc) const {
    auto s = resumeCycle();
    s->addr = pc;
    if (s->read() && s->memory())
        s->data = _mems->read(pc);
    return s;
}

// Serves the cycle the CPU waits in, raises READY, and latches the next
// cycle. Read data is driven while DBIN is high, and released once it
// falls; write data is valid while the CPU waits.
Signals *PinsI8080::completeCycle(Signals *s, const uint32_t *serviced) const {
    const auto read = s->read();
    if (read) {
        if (s->memory()) {
            // latchCycle() has read memory.
        } else if (s->ioRead()) {
            const uint8_t ioaddr = s->addr;
            if (_devs->isSelected(ioaddr))
                s->data = _devs->read(ioaddr);
        } else if (s->intAck()) {
            s->data = InstI8080::vec2Inst(_devs->vector());
        }
        s->outData();
    } else if (s->write()) {
        s->getData();
        if (s->memory()) {
            if (s->writeMemory())
                _mems->write(s->addr, s->data);
        } else if (s->ioWrite()) {
            const uint8_t ioaddr = s->addr;
            if (_devs->isSelected(ioaddr))
                _devs->write(ioaddr, s->data);
        }
    }
    // An interrupt from here to the next SYNC could miss its status.
    noInterrupts();
    raise_ready(phases());
    if (read) {
        wait_level<PIN_DBIN>(LOW, dbin_cycles);
        Signals::inputMode();
    }
    Cycles::next();
    latchCycle(serviced);
    interrupts();
    return s;
}

Signals *PinsI8080::inject(uint8_t data) const {
    return completeCycle(prepareCycle()->inject(data));
}

uint16_t PinsI8080::captureWrites(
        const uint8_t *inst, uint_fast8_t len, uint8_t *buf, uint_fast8_t max) {
    DEBUG(cli.print("@@ captureWrites: len="));
    DEBUG(cli.printDec(len));
    DEBUG(cli.print(" max="));
    DEBUG(cli.printlnDec(max));
    uint16_t addr = 0;
    uint_fast8_t inj = 0;
    uint_fast8_t cap = 0;
    auto s = resumeCycle();
    while (inj < len || cap < max) {
        if (inj < len)
            s->inject(inst[inj]);
        if (cap < max)
            s->capture();
        completeCycle(s);
        if (s->read()) {
            if (inj < len)
                ++inj;
        } else if (s->write()) {
            if (cap == 0)
                addr = s->addr;
            if (cap < max && buf)
                buf[cap++] = s->data;
        }
        DEBUG(cli.print("@@ captureWrites: inj="));
        DEBUG(cli.printDec(inj));
        DEBUG(cli.print(" cap="));
        DEBUG(cli.printDec(cap));
        DEBUG(cli.print(' '));
        DEBUG(s->print());
        s = prepareCycle();
    }
    return addr;
}

Signals *PinsI8080::loop() const {
    auto s = resumeCycle(_regs->nextIp());
    uint32_t serviced = ARM_DWT_CYCCNT;
    while (true) {
        completeCycle(s, &serviced);
        // The devices change INT only while the CPU waits.
        _devs->loop();
        s = prepareCycle();
        if (s->halt()) {
            const auto halt = s->prev();
            Cycles::discard(halt);
            return halt;
        }
        // The 8080's interrupt acknowledge is an M1 cycle too
        if (s->fetch() && !s->intAck() &&
                ARM_DWT_CYCCNT - serviced >= service_cycles) {
            serviced = ARM_DWT_CYCCNT;
            if (haltSwitch())
                return nullptr;
        }
    }
}

// Cycles::reset() rewinds the ring to slot 0, which keeps a stale cycle;
// the cycle the CPU waits in moves along, as its status was on the bus
// only during SYNC.
void PinsI8080::resetCycles() const {
    const auto waiting = *Signals::put();
    Cycles::reset();
    *Signals::put() = waiting;
}

void PinsI8080::run() {
    _regs->restore();
    resetCycles();
    saveBreakInsts();
    startRunTimer();
    const auto halt = loop();
    stopRunTimer();
    restoreBreakInsts();
    disassembleCycles();
    if (halt) {
        const auto pc = halt->addr;
        const auto inte = digitalReadFast(PIN_INTE);
        assert_reset();  // resume from HALT
        delayNanoseconds(reset_ns);
        negate_reset();
        noInterrupts();
        latchCycle();
        interrupts();
        // Registers, except PC and IE, are preserved
        regs<RegsI8080>()->saveContext(pc, inte);
    } else {
        _regs->save();
    }
}

bool PinsI8080::rawStep() const {
    auto s = resumeCycle(_regs->nextIp());
    const auto insn = _mems->read(s->addr);
    if (insn == InstI8080::HLT) {
        // Leave HLT unexecuted; run() resumes from HALT only by a reset.
        return false;
    }
    s->inject(insn);
    do {
        completeCycle(s);
        s = prepareCycle();
    } while (!s->fetch());
    return true;
}

bool PinsI8080::step(bool show) {
    resetCycles();
    _regs->restore();
    if (show)
        resetCycles();
    if (rawStep()) {
        if (show)
            printCycles();
        _regs->save();
        return true;
    }
    return false;
}

void PinsI8080::assertInt(uint8_t) {
    digitalWriteFast(PIN_INT, HIGH);
}

void PinsI8080::negateInt(uint8_t) {
    digitalWriteFast(PIN_INT, LOW);
}

void PinsI8080::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles; ++i) {
        g->next(i)->print();
    }
}

const SignalsImpl *PinsI8080::findBacktraceStart() {
    return backtraceStartByFetchCount<Signals>(_lineLimit);
}

void PinsI8080::printBacktrace() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles;) {
        const auto s = g->next(i);
        if (s->fetch() && !s->intAck()) {
            const auto next = _mems->disassemble(s->addr, 1);
            i += next - s->addr;
        } else {
            s->print();
            ++i;
        }
    }
}

}  // namespace i8080
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
