#include "pins_cp1610.h"
#include "debugger.h"
#include "devs_cp1610.h"
#include "digital_bus.h"
#include "inst_cp1610.h"
#include "mems_cp1610.h"
#include "regs_cp1610.h"
#include "signals_cp1610.h"

// #define DEBUG(e) e
#define DEBUG(e)

namespace debugger {
namespace cp1600 {

// clang-format off
/**
 * CP1610 bus cycle; a microcycle is TS1~TS4, two periods of PHI1/PHI2. A
 * read is BAR, a NACT microcycle whose TS1 samples BDRDY, then DTB; a
 * write is BAR, DW and DWS. The address is out from TS3 of BAR, and read
 * data is in by TS3 of DTB.
 *       | TS1 | TS2 | TS3 | TS4 | TS1 | TS2 | TS3 | TS4 | TS1 |...| TS4 |
 *        __          __          __          __          __
 * PHI1 _|  |________|  |________|  |________|  |________|  |__ ... ______
 *              __          __          __          __                __
 * PHI2 _______|  |________|  |________|  |________|  |________ ... _|  |_
 *       _______________________ _______________________ _______________
 * BCx  X__________BAR__________X__________NACT_________X_____DTB_______X
 *                   ___________                           __________
 * D0-15 -----------<__address__>-------------------------<__data____>--
 *       _____________________     _______________________________________
 * BDRDY                      \___/ sampled in TS1 after BAR/ADAR
 */
// clang-format on
// tCY: min 0.5 us, max 2 us; PHI1/PHI2 period
// tPHI: min 200 ns; PHI1/PHI2 pulse width, per the CP1600 Data Manual
// (the CP1610 data sheet says 250 ns, which 2 MHz leaves no gap for)
// tDC: max 100 ns; PHI1 to BDIR/BC1/BC2 valid
// tBO: max 100 ns; PHI1 to D0-D15 output
// tB1: min 0 ns; D0-D15 input setup to PHI1
// tB2: min 10 ns; D0-D15 input hold from PHI1
// tms: max 30 ns; PHI1 to #MSYNC
// tSKEW: min 0 ns; PHI1/PHI2 delay; tr/tf: max 15 ns
// BDRDY: low 50 ns before TS1 ends after BAR/ADAR, for at most 40 us
// tBU: 150 ns; PHI1 to BUSAK

// The period in counts of the 150 MHz bus clock: PHI1 high from 0, PHI2
// high from half a period, each as wide as tPHI allows. Past the CPU's
// limits, as K may set with no CPU fitted, the phases narrow to keep
// their gap.
struct Phases {
    uint16_t n;     // tCY
    uint16_t high;  // PHI1/PHI2 high width
    uint16_t on2;   // PHI2 high from here

    static constexpr uint16_t phi_high = (200 * 150 + 999) / 1000 + 1;
    static constexpr uint16_t phi_gap = 6;  // 40 ns
    static constexpr uint16_t highOf(uint16_t counts) {
        return counts / 2 >= phi_high + phi_gap ? phi_high
                                                : counts / 2 - phi_gap;
    }

    constexpr Phases(uint16_t counts)
        : n(counts), high(highOf(counts)), on2(counts / 2) {}
    uint32_t hz() const { return F_BUS_ACTUAL / n; }
};

namespace {

// 150 MHz / 75: tCY 500 ns, PHI1/PHI2 207 ns with 43 ns between.
constexpr Phases phi_phases{75};
// tCY max 2,000 ns.
constexpr uint32_t phi_min_hz = 500000;
// The narrowest K takes, for the clock drivers alone with no CPU fitted:
// 5 MHz, 60 ns phases.
constexpr uint16_t phi_min_counts = 30;
// PHI2's counter starts this many counts after PHI1's, the two restart
// writes apart; as measured on the P8080, whose clocks use the same pins.
constexpr uint16_t phi2_lag = 4;

using Phi1 = PwmClock<PIN_PHI1>;
using Phi2 = PwmClock<PIN_PHI2>;

// Teensy cycles in a count of the bus clock.
constexpr uint32_t cycles_per_count = F_CPU / 150000000;
// BDIR/BC1/BC2 are read this long into TS1: tDC 100 ns past PHI1 at the
// CPU, which follows PHI1 here by the driver's delay.
constexpr uint16_t code_counts = 24;  // 160 ns
// D0-D15 are read this long into PHI2 of TS4, as an address or write data.
constexpr uint16_t addr_counts = 10;  // 67 ns
// Read data is released this long after PHI2 of TS4 falls.
constexpr uint16_t release_counts = 3;  // 20 ns

// Waits until the Teensy cycle count reaches |t|.
inline void wait_until(uint32_t t) {
    while (int32_t(ARM_DWT_CYCCNT - t) < 0)
        ;
}

// The interrupt address injected while probing INTFF; nothing is fetched
// from it, every read is injected.
constexpr uint16_t VEC_DEBUG = 0x0000;
// Microcycles to wait for the CPU to stop, start or resume; an
// instruction takes far fewer.
constexpr auto halt_cycles = 256;
// The USART polls the console once in this many calls of its loop(), one
// for each transaction; that and the halt switch use USB, whose interrupts
// may outlast a BDRDY hold, so a run requests the bus for them as often.
constexpr uint32_t service_transactions = 256;

const uint8_t PINS_LOW[] = {
        PIN_PHI1,
        PIN_PHI2,
        PIN_MSYNC,
        PIN_EBCI,
};

const uint8_t PINS_HIGH[] = {
        PIN_INTRM,
        PIN_STPST,
        PIN_BDRDY,
        PIN_BUSREQ,
        PIN_ENSWI,
};

const uint8_t PINS_INPUT[] = {
        PIN_PCIT,
        PIN_BDIR,
        PIN_BC1,
        PIN_BC2,
        PIN_TCI,
        PIN_HALT,
        PIN_BUSACK,
        PIN_EBCA0,
        PIN_EBCA1,
        PIN_EBCA2,
        PIN_EBCA3,
        PIN_D0,
        PIN_D1,
        PIN_D2,
        PIN_D3,
        PIN_D4,
        PIN_D5,
        PIN_D6,
        PIN_D7,
        PIN_D8,
        PIN_D9,
        PIN_D10,
        PIN_D11,
        PIN_D12,
        PIN_D13,
        PIN_D14,
        PIN_D15,
};

// BDRDY low keeps the CPU in NACT after a BAR or ADAR, for 40 us at most.
constexpr uint32_t hold_max_cycles = F_CPU / 1000000 * 40;

void assert_bdrdy() {
    digitalWriteFast(PIN_BDRDY, LOW);
}

void negate_bdrdy() {
    digitalWriteFast(PIN_BDRDY, HIGH);
}

void assert_busrq() {
    digitalWriteFast(PIN_BUSREQ, LOW);
}

void negate_busrq() {
    digitalWriteFast(PIN_BUSREQ, HIGH);
}

auto busak_asserted() {
    return digitalReadFast(PIN_BUSACK) == LOW;
}

inline uint8_t bus_code() {
    return busRead(CNTL) & 7;
}

auto halt_asserted() {
    return digitalReadFast(PIN_HALT) != LOW;
}

void assert_stpst() {
    digitalWriteFast(PIN_STPST, LOW);
}

void negate_stpst() {
    digitalWriteFast(PIN_STPST, HIGH);
}

// #INTR is shared with the SIN trap logic, and has a pull-up. Released, the
// Teensy's own pull-up keeps the level shifter's input from floating.
void negate_intr() {
    pinMode(PIN_INTR, INPUT_PULLUP);
}

void intrm(bool assert) {
    digitalWriteFast(PIN_INTRM, assert ? LOW : HIGH);
}

}  // namespace

PinsCp1610::PinsCp1610()
    : _phi1(0, phi_min_hz),
      _phi2(0, phi_min_hz),
      _microcycle(0),
      _next(0),
      _ts1(0),
      _holding(false),
      _missed(0),
      _late(0),
      _held(0),
      _overheld(0),
      _addr(0),
      _prefetch(0),
      _prefetched(false),
      _remain(0),
      _sdbd(false),
      _branch(false),
      _disp(0),
      _intrm(false) {
    _devs = new DevsCp1610();
    auto mems = new MemsCp1610(_devs);
    _mems = mems;
    _regs = new RegsCp1610(this, mems);
#ifdef TUNE_CLOCK
    _counts = phi_phases.n;
#endif
}

const Phases &PinsCp1610::phases() const {
#ifdef TUNE_CLOCK
    static Phases p{phi_phases};
    if (p.n != _counts)
        p = Phases(_counts);
    return p;
#else
    return phi_phases;
#endif
}

// Both phases at the rate, and the two counters restarted together once
// the rate has loaded, at the end of the period under way: at boot the
// core's default, about 223 us.
void PinsCp1610::setPhases() const {
    const auto &p = phases();
    _phi1.setRate(p.hz());
    _phi2.setRate(p.hz());
    Phi1::setEdges(0, p.high);
    Phi2::setEdges(p.on2 - phi2_lag, p.on2 + p.high - phi2_lag);
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

// The bus timing points from the phases.
void PinsCp1610::setTiming() {
    const auto &p = phases();
    _microcycle = 2 * p.n * cycles_per_count;
    _addrAt = (p.n + p.on2 + addr_counts) * cycles_per_count;
    _releaseAt = (p.n + p.on2 + p.high + release_counts) * cycles_per_count;
}

#ifdef TUNE_CLOCK
// A new rate restarts the counters, which loses the microcycle's phase:
// the CPU is reset with it.
bool PinsCp1610::setClockInput(uint32_t hz) {
    if (hz < phi_min_hz)
        return false;
    const uint16_t counts = (F_BUS_ACTUAL + hz / 2) / hz;
    if (counts < phi_min_counts)
        return false;
    _counts = counts;
    resetPins();
    return true;
}

uint32_t PinsCp1610::clockInput() const {
    return phases().hz();
}
#endif

void PinsCp1610::resetPins() {
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
    negate_intr();
    _intrm = false;

    const auto &p = phases();
    _phi1.begin(p.hz());
    _phi2.begin(p.hz());
    setPhases();
    setTiming();
    _phi1.start();
    _phi2.start();
    // #MSYNC low for at least 10 ms, then high within tms of a PHI1 rise,
    // which becomes TS3: the next period is TS1.
    delay(10);
    const uint16_t half = p.n / 2;
    noInterrupts();
    while (Phi1::count() < half)
        ;
    while (Phi1::count() >= half)
        ;
    digitalWriteFast(PIN_MSYNC, HIGH);
    const auto now = ARM_DWT_CYCCNT;
    const auto count = Phi1::count();
    _next = now - count * cycles_per_count + p.n * cycles_per_count;
    _missed = _late = _overheld = 0;
    _holding = false;
    _prefetched = false;

    Cycles::reset();
    _remain = 0;
    _sdbd = false;
    _branch = false;
    _writes = true;
    // The CPU inputs its starting address with IAB.
    auto s = prepareCycle(halt_cycles);
    if (s == nullptr || s->bus() != IAB) {
        interrupts();
        cli.println("?reset: no IAB");
        return;
    }
    completeCycle(s->inject(InstCp1610::RESET_PC));
    // Hold the fetch from the start address until registers are saved.
    s = prepareCycle(halt_cycles, true);
    interrupts();
    if (s == nullptr || !_holding) {
        cli.println("?reset: no fetch");
        return;
    }
    reportMisses("reset");
    _regs->reset();
    _regs->save();
}

/**
 * Wait for the next microcycle's TS1 and read its bus control into
 * Signals::put(), with |_ts1| its start. Microcycles that passed while the
 * debugger was busy are skipped and counted.
 */
Signals *PinsCp1610::nextCode() {
    const auto behind = ARM_DWT_CYCCNT - _next;
    if (int32_t(behind) >= int32_t(_microcycle)) {
        const auto skip = behind / _microcycle;
        _missed += skip;
        _next += skip * _microcycle;
    }
    _ts1 = _next;
    _next += _microcycle;
    wait_until(_ts1 + code_counts * cycles_per_count);
    auto s = Signals::put();
    s->getControl();
    return s;
}

/**
 * Read memory at |_addr| ahead of the DTB, at least a microcycle away: a
 * read through Mems takes much of the time from TS1 to TS3. Reading may
 * change a device, so one is read ahead only for an instruction that reads
 * its operand; an MVO writes it.
 */
void PinsCp1610::prefetch() {
    _prefetched = !_writes || !_devs->isSelected(_addr);
    if (_prefetched)
        _prefetch = _mems->read(_addr);
    // Looking the break points up at the DTB may make its data late.
    _breakAhead = isBreakPoint(_addr);
}

// Latch the address a BAR outputs in TS3 and TS4.
void PinsCp1610::latchBar() {
    wait_until(_ts1 + _addrAt);
    _addr = busRead(DATA);
    prefetch();
}

/**
 * Let the CPU held by BDRDY go on: BDRDY rises in TS3 of a NACT microcycle
 * it waits in, clear of TS1 where the CPU samples it, so the next TS1
 * ends the wait.
 */
void PinsCp1610::release() {
    const auto period = _microcycle / 2;
    auto ts1 = _next;
    const auto since = ARM_DWT_CYCCNT - _next;
    if (int32_t(since) > 0)
        ts1 += (since / _microcycle) * _microcycle;
    if (int32_t(ARM_DWT_CYCCNT - (ts1 + period)) > 0)
        ts1 += _microcycle;
    wait_until(ts1 + period);
    negate_bdrdy();
    if (ARM_DWT_CYCCNT - _held > hold_max_cycles)
        ++_overheld;
    _next = ts1 + _microcycle;
    _holding = false;
}

/**
 * Follow the CPU until a transaction that carries data, and return it in
 * its TS1; BAR and DW are handled here. With |hold|, return a BAR instead,
 * with BDRDY holding the CPU until the next call. Returns nullptr if the
 * CPU halted, granted the bus with |busak|, or nothing came within |limit|
 * microcycles. Interrupts must be masked.
 */
Signals *PinsCp1610::prepareCycle(
        uint_fast16_t limit, bool hold, bool busak) {
    if (_holding)
        release();
    for (uint_fast16_t n = 0; n < limit; ++n) {
        auto s = nextCode();
        const auto bus = s->bus();
        if (bus == ADAR || bus == IAB || bus == DTB || bus == DWS ||
                bus == INTAK) {
            s->addr = _addr;
            return s;
        }
        if (bus == BAR) {
            if (hold) {
                assert_bdrdy();
                _held = ARM_DWT_CYCCNT;
                _holding = true;
            }
            latchBar();
            s->addr = _addr;
            if (hold)
                return s;
        }
        if (bus == NACT && (halt_asserted() || (busak && busak_asserted())))
            return nullptr;
    }
    return nullptr;
}

/**
 * Complete the transaction |s| that prepareCycle() returned in its TS1.
 */
Signals *PinsCp1610::completeCycle(Signals *s) {
    const auto bus = s->bus();
    if (s->read()) {
        if (s->readMemory()) {
            if (bus == IAB) {
                s->data = _vector;
            } else if (_prefetched && s->addr == _addr) {
                s->data = _prefetch;
            } else {
                s->data = _mems->read(s->addr);
            }
        }
        s->outData();
        _prefetched = false;
        // Read data is due by TS3, half a microcycle in.
        if (int32_t(ARM_DWT_CYCCNT - (_ts1 + _microcycle / 2)) > 0)
            ++_late;
        Cycles::next();
        wait_until(_ts1 + _releaseAt);
        Signals::inputMode();
        if (bus == ADAR) {
            _addr = s->data;
            prefetch();
        }
    } else {
        wait_until(_ts1 + _addrAt);
        if (bus == INTAK) {
            s->getAddr();  // R6
            s->data = _addr = s->addr;
            // Read ahead of the IAB, which is due by its TS3.
            _vector = _devs->vector();
        } else {
            s->getData();
        }
        Cycles::next();
        if (bus == DWS && s->writeMemory())
            _mems->write(s->addr, s->data);
    }
    trackFetch(s);
    return s;
}

void PinsCp1610::reportMisses(const char *where) {
    if (_missed == 0 && _late == 0 && _overheld == 0)
        return;
    cli.print("?");
    cli.print(where);
    cli.print(": missed ");
    cli.printDec(_missed);
    cli.print(" microcycles, late ");
    cli.printDec(_late);
    cli.print(" reads, held too long ");
    cli.printDec(_overheld);
    cli.println(" times");
    _missed = _late = _overheld = 0;
}

/**
 * The CP1610 has no fetch signal: count the transactions each fetched
 * instruction makes, and the next DTB after them is the next fetch.
 */
void PinsCp1610::trackFetch(Signals *s) {
    const auto bus = s->bus();
    if (bus == INTAK) {
        _remain = 2;  // DW/DWS of PC, then IAB
        _branch = false;
    } else if (fetching(s)) {
        s->markFetch();
        const auto opc = s->data & 0x3FF;
        _remain = InstCp1610::busCycles(s->data, _sdbd);
        _sdbd = opc == InstCp1610::SDBD;
        // A branch not taken skips its displacement.
        _branch = opc >= 0x200 && opc < 0x240;
        _writes = (opc & 0x3C0) == InstCp1610::MVO;
        _disp = s->addr + 1;
    } else if (_remain) {
        --_remain;
        _branch = false;
    }
}

bool PinsCp1610::fetching(const Signals *s) const {
    return s->bus() == DTB &&
           (_remain == 0 || (_branch && s->addr != _disp));
}

void PinsCp1610::idle() {
    // PHI1 and PHI2 run by themselves.
    regrid();
}

/**
 * Move |_next| up to the coming TS1. The microcycles keep their phase from
 * #MSYNC on, while the CPU waits on HALT or BUSRQ as well; the Teensy cycle
 * count wraps in 7 seconds, which idle() keeps well ahead of.
 */
void PinsCp1610::regrid() {
    const auto behind = ARM_DWT_CYCCNT - _next;
    if (int32_t(behind) > 0)
        _next += (behind / _microcycle + 1) * _microcycle;
}

void PinsCp1610::unhalt() {
    // #STPST is edge triggered; the CPU may start within the microcycle.
    // Keep the address of a BAR that comes before HALT goes low.
    assert_stpst();
    for (auto n = 0; n < halt_cycles && halt_asserted(); ++n) {
        const auto bar = nextCode()->bus() == BAR;
        negate_stpst();
        if (bar)
            latchBar();
    }
    negate_stpst();
}

/**
 * The fetch the CPU is held at by resumeInsts(): let it go on and return
 * the transaction. Interrupts must be masked.
 */
Signals *PinsCp1610::resumeCycle() {
    return prepareCycle();
}

void PinsCp1610::startInsts() {
    regrid();
    intrm(false);
    if (halt_asserted())
        unhalt();
    // Halted, or past an IAB: at an instruction boundary.
    _remain = 0;
    _sdbd = false;
    _branch = false;
    _writes = true;
}

/**
 * The |i|-th read after injected instructions: HLT stops the CPU only after
 * the next interruptible instruction, so NOP follows it.
 */
uint16_t PinsCp1610::parking(uint_fast8_t i) {
    return i == 0 ? InstCp1610::HLT : InstCp1610::NOP;
}

uint16_t PinsCp1610::injectInsts(const uint16_t *inst, uint_fast8_t len,
        uint16_t *buf, uint_fast8_t max) {
    noInterrupts();
    startInsts();
    uint16_t pc = 0;
    uint_fast8_t i = 0;
    uint_fast8_t w = 0;
    for (auto n = 0; n < halt_cycles; ++n) {
        auto s = prepareCycle();
        if (s == nullptr) {
            intrm(_intrm);
            interrupts();
            reportMisses("inject");
            return pc;  // halted
        }
        if (s->read()) {
            if (n == 0)
                pc = s->addr;
            s->inject(i < len ? inst[i] : parking(i - len));
            ++i;
        } else {
            s->capture();
        }
        completeCycle(s);
        if (s->bus() == DWS && w < max)
            buf[w++] = s->data;
        DEBUG(cli.print("@@  inject: "));
        DEBUG(s->print());
    }
    intrm(_intrm);
    interrupts();
    reportMisses("inject");
    cli.println("?inject: CPU did not halt");
    return pc;
}

bool PinsCp1610::probeIntff() {
    noInterrupts();
    startInsts();
    intrm(true);
    auto intff = false;
    uint_fast8_t i = 0;
    for (auto n = 0; n < halt_cycles; ++n) {
        auto s = prepareCycle();
        if (s == nullptr)
            break;  // halted
        if (s->bus() == INTAK) {
            intff = true;
            intrm(false);
        } else if (s->bus() == IAB) {
            s->inject(VEC_DEBUG);
        } else if (s->read()) {
            // The first fetch is NOP, which is interruptible.
            s->inject(i == 0 ? InstCp1610::NOP : parking(i - 1));
            ++i;
        } else {
            s->capture();
        }
        completeCycle(s);
        DEBUG(cli.print("@@   probe: "));
        DEBUG(s->print());
    }
    intrm(_intrm);
    interrupts();
    reportMisses("probe");
    return intff;
}

void PinsCp1610::resumeInsts(const uint16_t *inst, uint_fast8_t len) {
    noInterrupts();
    startInsts();
    uint_fast8_t i = 0;
    for (auto n = 0; n < halt_cycles; ++n) {
        // Past the last injected word, BDRDY holds the next fetch for
        // resumeCycle().
        auto s = prepareCycle(64, i == len);
        if (s == nullptr)
            break;
        if (i == len) {
            intrm(_intrm);
            interrupts();
            reportMisses("resume");
            if (!_holding)
                cli.println("?resume: fetch not held");
            return;
        }
        if (s->read()) {
            s->inject(inst[i++]);
        } else {
            s->capture();
        }
        completeCycle(s);
        DEBUG(cli.print("@@  resume: "));
        DEBUG(s->print());
    }
    intrm(_intrm);
    interrupts();
    cli.println("?resume: CPU did not resume");
}

/**
 * The CPU stopped after a HLT and the interruptible instruction following
 * it, and its next fetch is the PC: save the registers.
 */
void PinsCp1610::halted() {
    Cycles::Hold hold;
    _regs->save();
}

bool PinsCp1610::loop() {
    noInterrupts();
    auto s = resumeCycle();
    auto stopping = 0;
    auto requested = false;
    // At a break point, HLT and NOP are fetched instead, to stop the CPU
    // before it.
    auto parked = 0;
    uint16_t brk = 0;
    // The devices count their own time in calls of loop(), one for each
    // transaction, as when they ran between transactions.
    uint32_t transactions = 0;
    while (true) {
        if (s == nullptr) {
            negate_busrq();
            // The CPU may stop before #STPST goes high again, and the
            // register save restarts it with the next falling edge.
            negate_stpst();
            interrupts();
            reportMisses("run");
            if (halt_asserted()) {
                halted();
                if (parked)
                    regs<RegsCp1610>()->stoppedAt(brk);
                return true;
            }
            cli.println("?run: no bus transaction");
            return false;
        }
        if (fetching(s)) {
            if (parked) {
                s->inject(InstCp1610::NOP);
            } else if (stopping == 0 && _breakAhead) {
                s->inject(InstCp1610::HLT);
                brk = s->addr;
                parked = 1;
            }
        }
        completeCycle(s);
        ++transactions;
        if (stopping && ++stopping > halt_cycles) {
            negate_stpst();
            interrupts();
            reportMisses("run");
            cli.println("?halt: CPU did not stop");
            return false;
        }
        if (stopping == 2)
            negate_stpst();
        if (!requested && transactions >= service_transactions) {
            // Granted at the end of an interruptible instruction.
            assert_busrq();
            requested = true;
        }
        s = prepareCycle(64, false, requested);
        if (s == nullptr && requested && busak_asserted()) {
            interrupts();
            do {
                _devs->loop();
            } while (transactions && --transactions);
            const auto halt = stopping == 0 && haltSwitch();
            noInterrupts();
            if (halt) {
                // The CPU stops after the instruction under way.
                assert_stpst();
                stopping = 1;
            }
            negate_busrq();
            requested = false;
            regrid();
            // BUSAK rises up to tBU into the microcycle the CPU goes on
            // in, which may be an INTAK as well as a BAR.
            s = prepareCycle(halt_cycles);
        }
    }
}

void PinsCp1610::run() {
    _regs->restore();
    Cycles::reset();
    saveBreakInsts();
    startRunTimer();
    loop();
    stopRunTimer();
    // context has been saved in loop()
    restoreBreakInsts();
    disassembleCycles();
}

/**
 * Let the CPU held by resumeInsts() go and stop it again with #STPST: it
 * stops after the instruction under way, or after the next interruptible
 * one.
 */
bool PinsCp1610::rawStep() {
    noInterrupts();
    auto s = resumeCycle();
    assert_stpst();
    for (auto n = 0; n < halt_cycles && s != nullptr; ++n) {
        completeCycle(s);
        if (n == 0)
            negate_stpst();
        s = prepareCycle();
    }
    negate_stpst();
    interrupts();
    reportMisses("step");
    if (s == nullptr && halt_asserted()) {
        halted();
        return true;
    }
    cli.println("?step: CPU did not stop");
    return false;
}

bool PinsCp1610::step(bool show) {
    Cycles::reset();
    _regs->restore();
    if (show)
        Cycles::reset();
    const auto stepped = rawStep();
    if (stepped && show)
        printCycles();
    // context has been saved by rawStep()
    return stepped;
}

void PinsCp1610::assertInt(uint8_t) {
    _intrm = true;
    intrm(true);
}

void PinsCp1610::negateInt(uint8_t) {
    _intrm = false;
    intrm(false);
}

// A break point's fetch is served HLT and NOP instead, so memory keeps the
// instruction there.
void PinsCp1610::setBreakInst(uint32_t) const {}

void PinsCp1610::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

const SignalsImpl *PinsCp1610::findBacktraceStart() {
    return backtraceStartByFetchCount<Signals>(_lineLimit);
}

void PinsCp1610::printBacktrace() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles;) {
        const auto s = g->next(i);
        if (s->fetch()) {
            const auto len = _mems->disassemble(s->addr, 1) - s->addr;
            // print bus cycles other than instruction words; a branch not
            // taken skips its displacement, so the next fetch may come first
            uint32_t j = 1;
            for (; j < len && i + j < cycles; j++) {
                const auto t = s->next(j);
                if (t->fetch())
                    break;
                if (t->addr < s->addr || t->addr >= s->addr + len) {
                    t->print();
                    idle();
                }
            }
            i += j;
        } else {
            s->print();
            ++i;
        }
        idle();
    }
}

}  // namespace cp1600
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
