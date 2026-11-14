#include "pins_dcj11.h"
#include "debugger.h"
#include "devs_pdp11.h"
#include "digital_bus.h"
#include "inst_pdp11.h"
#include "mems_pdp11.h"
#include "regs_pdp11.h"
#include "signals_dcj11.h"
#include "watchdog.h"

namespace debugger {
namespace dcj11 {

using pdp11::DevsPdp11;
using pdp11::InstPdp11;
using pdp11::MemsPdp11;
using pdp11::RegsPdp11;

// clang-format off
/**
 * DCJ11 bus cycles (DCJ11 User's Guide, Chapter 3). A cycle is four CLK
 * periods, T0-T3; a stretched one adds T4 pairs until #CONT, sampled at
 * T4's falling edges, ends it. The board holds #MISS low, so memory
 * reads stretch too; NIO cycles never do.
 *
 * The board drives CLK1 with #TEST2 grounded. A cycle is counted from
 * the period #ALE falls in; the board stops the clock high there to park
 * the CPU.
 *          ________                ______________
 * DAL     X__addr__X--------------<__read data___>------  the board drives
 *          ________  ____________________________
 * DAL     X__addr__><________write data__________>------  the CPU drives
 *         ____    ______________________________________
 * #ALE        |__|
 *         ________________                      ________
 * #SCTL                   |____________________|
 *         ________                                 _____
 * #BUFCTL         |_______________________________|        a read
 *         ________________________          ____________
 * #CONT                           |________|
 *
 * The address, AIO3-AIO0 and BS1-BS0 are valid while #ALE is low. Read
 * data is latched at T3's rise and with DV; write data is valid while
 * #SCTL is low.
 */
// clang-format on

namespace {

// A half period with no work gets these: the work between edges keeps
// the period above 100ns and each half above 35ns, inside the 20MHz part.
constexpr auto clock_lo_ns = 20;
constexpr auto clock_hi_ns = 25;
constexpr auto lo_ns = clock_lo_ns;
constexpr auto hi_ns = clock_hi_ns;
// A run's PWM: the address leaves DAL about a period after #ALE falls,
// and the read of it and the mux takes about 70ns; 8.33MHz misses it.
// The data chip's maximum is 20MHz (J-11 Data Chip Specification).
constexpr uint32_t pwm_hz = 7500000;
constexpr uint32_t pwm_min_hz = 100000;
constexpr uint32_t pwm_half_min_ns = 25;
// A run's wait for a cycle other than NIO before it looks at the halt
// switch, as in a WAIT; long enough for any FP11 instruction's NIO run.
constexpr uint32_t wait_cycles = F_CPU / 1000;
// How long #SCTL may take to come or go under the PWM.
constexpr uint32_t sctl_cycles = F_CPU / 1000000;
// Periods without a cycle other than NIO before a run counts the CPU as
// in WAIT.
constexpr auto no_ale_periods = 1024;
// Periods a strobe may take to come within a cycle.
constexpr auto strobe_periods = 32;
// #INIT is held at least 25 periods.
constexpr auto init_periods = 64;
// Cycles of the power-up microroutine before the first fetch.
constexpr auto power_up_cycles = 256;
// Bus cycles allowed for a halt or a step to reach the debugger.
constexpr auto trap_cycles = 256;
// Cycles back from the restart fetch the trap frame's writes may lie.
constexpr auto frame_cycles = 16;
// The 74HCT259's address setup and enable pulse.
constexpr auto latch_ns = 30;

// The power-up configuration (DCJ11 User's Guide, Figure 8-3): POK,
// power-up mode 2 (start at 173000), and a kernel HALT traps to 4.
constexpr uint16_t POWER_UP_CONFIG = 0000015;
constexpr uint16_t ORG_START = 0173000;
// The debugger's traps go here; the debugger answers its fetch.
constexpr uint16_t ORG_RESTART = 0173004;
constexpr uint16_t VEC_PWRFAIL = 0000024;

// The 74HCT259's outputs.
constexpr uint8_t Q_PWRF = 0;
constexpr uint8_t Q_EVENT = 1;
constexpr uint8_t Q_HALT = 2;
constexpr uint8_t Q_IRQ0 = 3;
constexpr uint8_t Q_IRQ3 = 6;
constexpr uint8_t Q_MISS = 7;

const uint8_t PINS_HIGH[] = {
        PIN_CLK,
        PIN_INIT,
        PIN_CONT,
        PIN_LSTRB,
        PIN_ASEL0,
        PIN_DV,
};

const uint8_t PINS_LOW[] = {
        PIN_LDATA,
        PIN_SEL2,
        PIN_ASEL1,
};

const uint8_t PINS_INPUT[] = {
        PIN_ALE,
        PIN_SCTL,
        PIN_BUFCTL,
        PIN_MAP,
        PIN_PRDC,
        PIN_STRB,
        PIN_MUX0,
        PIN_MUX1,
        PIN_MUX2,
        PIN_MUX3,
        PIN_DAL0,
        PIN_DAL1,
        PIN_DAL2,
        PIN_DAL3,
        PIN_DAL4,
        PIN_DAL5,
        PIN_DAL6,
        PIN_DAL7,
        PIN_DAL8,
        PIN_DAL9,
        PIN_DAL10,
        PIN_DAL11,
        PIN_DAL12,
        PIN_DAL13,
        PIN_DAL14,
        PIN_DAL15,
};

// One period from the falling edge, ending high.
void clock_cycle() {
    digitalWriteFast(PIN_CLK, LOW);
    delayNanoseconds(lo_ns);
    digitalWriteFast(PIN_CLK, HIGH);
    delayNanoseconds(hi_ns);
}

void assert_init() {
    digitalWriteFast(PIN_INIT, LOW);
}

void negate_init() {
    digitalWriteFast(PIN_INIT, HIGH);
}

void assert_cont() {
    digitalWriteFast(PIN_CONT, LOW);
}

void negate_cont() {
    digitalWriteFast(PIN_CONT, HIGH);
}

// DV is low active: the read latch is open while it is high and closes
// as it falls (J-11 Data Chip Specification, MDV-L).
void assert_dv() {
    digitalWriteFast(PIN_DV, LOW);
}

void negate_dv() {
    digitalWriteFast(PIN_DV, HIGH);
}

// digitalReadFast() may return the port bit, not 1: compare with LOW.
inline bool ale_asserted() {
    return digitalReadFast(PIN_ALE) == LOW;
}

inline bool sctl_asserted() {
    return digitalReadFast(PIN_SCTL) == LOW;
}

inline bool bufctl_asserted() {
    return digitalReadFast(PIN_BUFCTL) == LOW;
}

inline bool abort_asserted() {
    return digitalReadFast(PIN_ABORT) == LOW;
}

// Sets output |q| of the 259 to |level|, and leaves the 153s at select 1.
void write_latch(uint8_t q, bool level) {
    digitalWriteFast(PIN_ASEL0, (q & 1) ? HIGH : LOW);
    digitalWriteFast(PIN_ASEL1, (q & 2) ? HIGH : LOW);
    digitalWriteFast(PIN_SEL2, (q & 4) ? HIGH : LOW);
    digitalWriteFast(PIN_LDATA, level ? HIGH : LOW);
    delayNanoseconds(latch_ns);
    digitalWriteFast(PIN_LSTRB, LOW);
    delayNanoseconds(latch_ns);
    digitalWriteFast(PIN_LSTRB, HIGH);
    digitalWriteFast(PIN_ASEL1, LOW);
    digitalWriteFast(PIN_ASEL0, HIGH);
}

void assert_pwrf() {
    write_latch(Q_PWRF, LOW);
}

void negate_pwrf() {
    write_latch(Q_PWRF, HIGH);
}

// Clocks until |cond| holds, maybe not at all; false if it doesn't
// come. A period too many may pass the next #ALE.
template <typename COND>
bool clockUntil(COND cond) {
    for (auto guard = strobe_periods; guard > 0; --guard) {
        if (cond())
            return true;
        clock_cycle();
    }
    return cond();
}

}  // namespace

PinsDcj11::PinsDcj11() : _clock(pwm_half_min_ns, pwm_min_hz), _clockHz(pwm_hz) {
    _devs = new DevsPdp11();
    _mems = new MemsPdp11();
    _regs = new RegsPdp11(this, true);
}

#ifdef TUNE_CLOCK
uint32_t PinsDcj11::clockInput() const {
    return _clockHz;
}

// A run's PWM rate.
bool PinsDcj11::setClockInput(uint32_t hz) {
    const auto rate = _clock.setRate(hz);
    if (rate == 0)
        return false;
    _clockHz = rate;
    return true;
}
#endif

void PinsDcj11::resetPins() {
    _clock.begin(_clockHz);
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
    // The DCJ11's pull-up alone lets #ABORT glitch through the TXS0108E.
    pinMode(PIN_ABORT, INPUT_PULLUP);

    // The 259 powers up random: set every output while #INIT is low.
    assert_init();
    write_latch(Q_PWRF, HIGH);
    write_latch(Q_EVENT, HIGH);
    write_latch(Q_HALT, LOW);  // HALT is active high
    for (auto q = Q_IRQ0; q <= Q_IRQ3; ++q)
        write_latch(q, LOW);
    write_latch(Q_MISS, LOW);
    for (auto i = 0; i < init_periods; i++)
        clock_cycle();
    negate_init();
    Cycles::reset();
    _devPending = false;
    _trapVector = 0;

    // GP writes, the configuration read, register writes and memory
    // probes, then the first fetch.
    Signals *s = nullptr;
    for (auto guard = power_up_cycles; guard > 0; --guard) {
        s = prepareCycle();
        if (s && s->fetch() && s->addr16() == ORG_START)
            break;
        if (s)
            completeCycle(s);
    }
    if (s == nullptr || !s->fetch() || s->addr16() != ORG_START) {
        cli.print("?reset: fetch ");
        if (s)
            s->print();
        else
            cli.println();
        return;
    }
    _regs->setIp(ORG_START);
    _regs->save();
}

// Clocks to the next cycle that isn't NIO, parking at the end of the
// period #ALE falls in; the address stays valid through T1. #ALE may be
// shorter than a period, so both halves look for it. Null when none
// comes, as in a WAIT.
Signals *PinsDcj11::prepareCycle() {
    // completeCycle() returns after #ALE rose, so a low one is new.
    bool armed = true;
    bool seen = ale_asserted();
    for (auto guard = no_ale_periods; guard > 0; --guard) {
        if (seen && armed) {
            auto s = Signals::put();
            s->clearMark();  // Cycles::next() keeps the slot's old mark
            s->getAddr();
            if (!s->nio()) {
                _parked = *s;
                return s;
            }
            armed = false;
        } else if (!seen) {
            armed = true;
        }
        digitalWriteFast(PIN_CLK, LOW);
        delayNanoseconds(lo_ns);
        seen = ale_asserted();
        digitalWriteFast(PIN_CLK, HIGH);
        delayNanoseconds(hi_ns);
        seen = seen || ale_asserted();
    }
    return nullptr;
}

// Takes the parked cycle up again, at #ALE or in its stretch.
Signals *PinsDcj11::resumeCycle() {
    auto s = Signals::put();
    *s = _parked;
    s->clear();
    return s;
}

Signals *PinsDcj11::completeCycle(Signals *s) {
    settleDevRead(s);
    // GP and IACK cycles are known by AIO alone; their BS is undefined.
    const auto external = s->iack() || s->gpRead() || !s->internal();
    if (s->write() || s->gpWrite()) {
        if (!clockUntil(sctl_asserted))
            cli.println("?#SCTL write");
        s->getData();
        storeData(s);
    } else if (external) {
        if (s->iack()) {
            s->data = _devs->vector();
        } else if (s->gpRead()) {
            const auto code = s->addr & 0377;
            s->data = (code == 0 || code == 2) ? POWER_UP_CONFIG : 0;
        } else {
            retrieveData(s);
        }
        // The CPU lets DAL go with #BUFCTL, and has by the stretch; read
        // data is latched at T3's rise, and again as DV falls, held low
        // until the stretch ends.
        if (!clockUntil([] { return bufctl_asserted() || sctl_asserted(); }))
            cli.println("?#BUFCTL");
        s->outData();
        if (!clockUntil(sctl_asserted))
            cli.println("?#SCTL read");
        clock_cycle();
        assert_dv();
    } else if (!clockUntil(sctl_asserted)) {
        cli.println("?#SCTL internal");
    }
    // #BUFCTL rises only with the next cycle's address, about when its
    // #ALE comes: let DAL go once #SCTL rises, without waiting for it.
    if (abort_asserted()) {
        cli.print("?#ABORT ");
        s->print();
    }
    assert_cont();
    if (!clockUntil([] { return !sctl_asserted(); }))
        cli.println("?#CONT");
    negate_cont();
    negate_dv();
    s->inputMode();
    Cycles::next();
    return s;
}

// Reads are whole words; the CPU picks the byte it needs. The system
// registers read 0.
void PinsDcj11::retrieveData(Signals *s) {
    if (!s->readMemory())
        return;
    const uint16_t addr = s->addr16() & ~1;
    const auto devs = this->devs<DevsPdp11>();
    if (s->ioPage() && devs->isSelected(addr)) {
        s->data = devs->peek(addr);
        _devRead = addr;
        _devPending = true;
    } else if (s->memory() || s->ioPage()) {
        // The little-endian word in one read.
        s->data = mems<MemsPdp11>()->read_word(addr / 2);
    } else {
        s->data = 0;
    }
}

// A byte write drives the byte on the half its address picks.
void PinsDcj11::storeData(Signals *s) {
    if (!s->write() || !s->writeMemory())
        return;
    if (!s->memory() && !s->ioPage())
        return;
    const uint16_t addr = s->addr16() & ~1;
    if (s->ioPage() && _devs->isSelected(addr)) {
        uint16_t val = s->data;
        if (s->byteWrite())
            val = s->writeLow() ? lo(s->data) : hi(s->data);
        _devs->write(addr, val);
    } else if (!s->byteWrite()) {
        _mems->write16(addr, s->data);
    } else if (s->writeLow()) {
        _mems->write_byte(addr, lo(s->data));
    } else {
        _mems->write_byte(addr | 1, hi(s->data));
    }
}

// A device read takes effect unless the cycle after it writes the same
// word: that read was a read-modify-write's.
void PinsDcj11::settleDevRead(const Signals *s) {
    if (!_devPending)
        return;
    _devPending = false;
    if (s->write() && (s->addr16() & ~1) == _devRead)
        return;
    _devs->read(_devRead);
}

void PinsDcj11::execute(const uint16_t *inst, uint_fast8_t words,
        uint16_t *buf, uint_fast8_t max, uint16_t &org, uint32_t exit,
        const uint16_t *stack, uint16_t stackAt, uint_fast8_t stackWords) {
    const uint16_t bytes = 2 * words;
    uint_fast8_t cap = 0;
    // The exit counts only once the sequence's last word was read: it
    // may lie inside the sequence (save() returns to its own start).
    bool whole = false;
    auto s = resumeCycle();
    // Bound the damage: a wrong dump beats a hung debugger.
    for (auto guard = Cycles::MAX_CYCLES; s && guard > 0; --guard) {
        const uint16_t addr = s->addr16();
        const uint16_t off = addr - org;
        // A prefetch past the sequence may come before its last
        // instruction is done; only a demand read ends it.
        if (s->fetch() && whole && cap >= max &&
                (addr == exit ||
                        (exit == EXIT_END && off >= bytes && !s->request())))
            break;
        if (s->read()) {
            const uint16_t at = addr - stackAt;
            if (off < bytes) {
                s->inject(inst[off / 2]);
                if (off / 2 + 1u == words)
                    whole = true;
            } else if (stack && at < 2u * stackWords) {
                s->inject(stack[at / 2]);
            }
        } else if (s->write() && cap < max) {
            s->capture();
        }
        const auto capturing = s->write() && !s->writeMemory();
        completeCycle(s);
        if (capturing && buf) {
            buf[cap] = s->writeHigh() ? hi(s->data) : s->data;
            ++cap;
        }
        s = prepareCycle();
    }
    // Parked in it; hand the caller where that is.
    if (s)
        org = s->addr16();
}

void PinsDcj11::execInst(const uint16_t *inst, uint_fast8_t words,
        uint16_t &org, uint32_t exit, const uint16_t *stack, uint16_t stackAt,
        uint_fast8_t stackWords) {
    execute(inst, words, nullptr, 0, org, exit, stack, stackAt, stackWords);
}

void PinsDcj11::captureWrites(const uint16_t *inst, uint_fast8_t words,
        uint16_t *buf, uint_fast8_t max, uint16_t &org) {
    execute(inst, words, buf, max, org, org, nullptr, 0, 0);
}

// Whether |s|, a prepared cycle, enters the debugger. BPT and a T-bit
// trace read the vector at 14, #PWRF the one at 24: the debugger answers
// with the restart address and priority 7, and parks at its fetch. The
// PC and PS the trap pushed are the two latest writes. A BPT's PC is its
// own unless |step| ran it.
bool PinsDcj11::parkAfterFrame(Signals *s, bool step) {
    if (s->read() && !s->fetch() && s->memory()) {
        const uint16_t addr = s->addr16();
        if (addr == InstPdp11::VEC_BPT || addr == VEC_PWRFAIL) {
            s->inject(ORG_RESTART);
            _trapVector = addr;
            // Still asserted, #PWRF would trap again before the fetch.
            if (addr == VEC_PWRFAIL)
                negate_pwrf();
        } else if (_trapVector && addr == _trapVector + 2) {
            s->inject(InstPdp11::PSW_PRI7);
        }
        return false;
    }
    if (_trapVector == 0 || !s->fetch() || s->addr16() != ORG_RESTART)
        return false;
    const auto vector = _trapVector;
    _trapVector = 0;
    const Signals *frame[2] = {nullptr, nullptr};
    const Signals *first = nullptr;
    const auto back = Cycles::cycles() < frame_cycles ? Cycles::cycles()
                                                      : frame_cycles;
    for (auto i = 1U; i <= back; ++i) {
        const auto w = s->prev(i);
        if (w->read() && w->addr16() == vector)
            first = w;
        if (w->write() && w->memory() && frame[1] == nullptr) {
            frame[frame[0] ? 1 : 0] = w;
            first = w;
        }
    }
    if (frame[1] == nullptr)
        return false;
    // The PC lies below the PS, by the 16-bit addresses: the debugger runs
    // with the MMU off.
    const auto pc =
            frame[0]->addr16() < frame[1]->addr16() ? frame[0] : frame[1];
    const auto ps = pc == frame[0] ? frame[1] : frame[0];
    if (uint16_t(pc->addr16() + 2) != ps->addr16())
        return false;
    const uint16_t at =
            (vector == InstPdp11::VEC_BPT && !step) ? pc->data - 2 : pc->data;
    // A trace trap pushed the T bit the step set.
    const uint16_t psw = ps->data & ~InstPdp11::PSW_T;
    Cycles::discard(first);
    regs<RegsPdp11>()->parkInTrap(at, psw, ORG_RESTART);
    return true;
}

// Raises #PWRF on |s|, a prepared cycle, or on a CPU in WAIT when null:
// the instruction finishes, then the trap through 24.
bool PinsDcj11::suspend(Signals *s) {
    assert_pwrf();
    for (auto guard = trap_cycles; guard > 0; --guard) {
        if (s) {
            if (parkAfterFrame(s, false))
                return true;
            completeCycle(s);
        }
        s = prepareCycle();
    }
    negate_pwrf();
    _trapVector = 0;
    return false;
}

bool PinsDcj11::rawStep() {
    // A BPT or HALT traps with the trace trap still pending.
    const auto regs = this->regs<RegsPdp11>();
    const auto inst = _mems->read(regs->nextIp());
    if (inst == InstPdp11::BPT || inst == InstPdp11::HALT)
        return false;
    auto s = resumeCycle();
    for (auto guard = trap_cycles; guard > 0; --guard) {
        completeCycle(s);
        s = prepareCycle();
        if (s == nullptr)
            break;  // in WAIT
        if (parkAfterFrame(s, true))
            return true;
    }
    return suspend(s);
}

bool PinsDcj11::step(bool show) {
    Cycles::reset();
    auto regs = this->regs<RegsPdp11>();
    // The step's trace trap pushes a frame; SP is left as the program has
    // it, which after power-up may be anywhere.
    if (regs->kernelStackLow()) {
        cli.println("?step: SP below 000400");
        return false;
    }
    regs->setTrace(true);
    _regs->restore();
    regs->setTrace(false);
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

// The PWM takes CLK1 with the CPU held in a stretch, and gives it back
// held again; the masked output is low.
void PinsDcj11::startClock() {
    _clock.start();
}

void PinsDcj11::stopClock() {
    digitalWriteFast(PIN_CLK, LOW);  // the GPIO takes the pad back low
    _clock.stop(_clockHz);
}

// Under the PWM: the next cycle that isn't NIO within |cycles|, its
// address taken at #ALE, with interrupts masked. Null when none comes.
Signals *PinsDcj11::catchCycle(uint32_t cycles) {
    const auto start = ARM_DWT_CYCCNT;
    for (;;) {
        // Twice low: a coupled spike lasts a few ns.
        while (!(ale_asserted() && ale_asserted())) {
            if (ARM_DWT_CYCCNT - start >= cycles)
                return nullptr;
        }
        auto s = Signals::put();
        s->clearMark();
        s->getRunAddr();
        if (!s->nio()) {
            return s;
        }
        // A NIO's #ALE stays low on a clock the mask stopped.
        while (ale_asserted()) {
            if (ARM_DWT_CYCCNT - start >= cycles)
                return nullptr;
        }
    }
}

// The next cycle, interrupts masked from the last release until its
// address is in; the CPU then runs on to its stretch, which holds it,
// while the debugger gets its data ready. Null, with interrupts still
// masked, when none comes for a while, as in a WAIT.
Signals *PinsDcj11::waitCycle() {
    noInterrupts();
    auto s = catchCycle(wait_cycles);
    if (s == nullptr)
        return nullptr;
    interrupts();
    return s;
}

// Stops the PWM with interrupts masked, catching a cycle that begins
// before the mask takes hold; once the clock is stopped none can, and
// interrupts may come. A caught cycle is bit-banged into its stretch.
Signals *PinsDcj11::stopWatching() {
    Clock::mask(true);
    // The mask holds from the next period start.
    auto s = catchCycle(2 * F_CPU / _clockHz);
    stopClock();
    if (s)
        clockUntil(sctl_asserted);
    return s;
}

// A cycle's data, as completeCycle() moves it. A read's goes on DAL once
// #BUFCTL lets it, before T3's rise, which latches it (DCJ11 User's
// Guide, 3.2); DV latches it again in the stretch.
void PinsDcj11::serve(Signals *s) {
    settleDevRead(s);
    const auto read = !s->write() && !s->gpWrite() &&
                      (s->iack() || s->gpRead() || !s->internal());
    const auto start = ARM_DWT_CYCCNT;
    if (read) {
        if (s->iack()) {
            s->data = _devs->vector();
        } else if (s->gpRead()) {
            const auto code = s->addr & 0377;
            s->data = (code == 0 || code == 2) ? POWER_UP_CONFIG : 0;
        } else {
            retrieveData(s);
        }
        while (!bufctl_asserted() && !sctl_asserted() &&
                ARM_DWT_CYCCNT - start < sctl_cycles)
            ;
        s->outData();
    }
    while (!sctl_asserted() && ARM_DWT_CYCCNT - start < sctl_cycles)
        ;
    if (s->write() || s->gpWrite()) {
        s->getData();
        storeData(s);
    } else if (read) {
        assert_dv();
    }
}

// Ends a held cycle with #CONT; interrupts stay masked until the next
// address is in.
void PinsDcj11::release(Signals *s) {
    if (abort_asserted()) {
        cli.print("?#ABORT ");
        s->print();
    }
    noInterrupts();
    assert_cont();
    const auto start = ARM_DWT_CYCCNT;
    while (sctl_asserted() && ARM_DWT_CYCCNT - start < sctl_cycles)
        ;
    negate_cont();
    negate_dv();
    s->inputMode();
    Cycles::next();
}

// Runs under the PWM until a BPT or the halt switch, every cycle held in
// its stretch while it is served. Returns whether the registers can be
// saved.
bool PinsDcj11::loop() {
    Signals *s = resumeCycle();
    clockUntil(sctl_asserted);
    startClock();
    for (uint_fast8_t n = 0;; ++n) {
        // Quiet, as in a WAIT: interrupts come only on a stopped clock.
        const auto quiet = s == nullptr;
        if (quiet) {
            s = stopWatching();
            interrupts();
            // A WAIT ticks the watchdog only once per quiet stretch.
            Watchdog::feed();
        }
        if (s && parkAfterFrame(s, false)) {
            if (!quiet)
                stopClock();
            _parked = *s;  // where the next run resumes
            return true;
        }
        // The devices' slow service comes round on its own clock; its
        // check costs a cycle a hundred ns, so not every one.
        const auto serviced = ((n & 15) == 0 || quiet) && _devs->loop();
        if ((serviced || quiet) && haltSwitch()) {
            if (!quiet)
                stopClock();
            if (s)
                _parked = *s;  // suspend() may park in it
            return suspend(s);
        }
        if (quiet) {
            noInterrupts();
            startClock();
        }
        if (s) {
            serve(s);
            release(s);
        }
        s = waitCycle();
    }
}

void PinsDcj11::run() {
    _regs->restore();
    Cycles::reset();
    saveBreakInsts();
    startRunTimer();
    const auto stopped = loop();
    stopRunTimer();
    restoreBreakInsts();
    if (stopped) {
        Cycles::Hold hold;
        _regs->save();
    }
    disassembleCycles();
}

void PinsDcj11::setBreakInst(uint32_t addr) const {
    _mems->put_prog(addr, InstPdp11::BPT);
}

// Every request is IRQ0, level 4; the device's vector is served at the
// IACK.
void PinsDcj11::assertInt(uint8_t) {
    write_latch(Q_IRQ0, HIGH);
}

void PinsDcj11::negateInt(uint8_t) {
    write_latch(Q_IRQ0, LOW);
}

void PinsDcj11::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

// Discarded prefetches and operand words are fetch()es too, so the
// instructions are counted as printBacktrace() walks them.
const SignalsImpl *PinsDcj11::findBacktraceStart() {
    const auto g = Signals::get();
    const auto insns = walkInsns(UINT32_MAX);
    if (insns <= _lineLimit)
        return g;
    return g->next(walkInsns(insns - _lineLimit));
}

// The cycle the |nth| instruction printBacktrace() shows starts at, or
// with no such one, how many there are.
uint32_t PinsDcj11::walkInsns(uint32_t nth) const {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    const auto mems = this->mems<MemsPdp11>();
    uint32_t from = 0, to = 0, insns = 0;
    for (auto i = 0u; i < cycles; ++i) {
        const auto s = g->next(i);
        const uint16_t addr = s->addr16();
        if (s->fetch() && (addr < from || addr >= to) && !discarded(i)) {
            if (insns++ == nth)
                return i;
            from = addr;
            to = addr + mems->insnLength(addr);
        }
    }
    return insns;
}

// An IREQ the CPU threw away: a demand fetch comes next, as after a
// taken branch, JMP, JSR, RTS or SOB, or none does, as the prefetch past
// the stop PC.
bool PinsDcj11::discarded(uint32_t i) const {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    if (!g->next(i)->request())
        return false;
    while (++i < cycles) {
        const auto s = g->next(i);
        if (s->fetch())
            return !s->request();
    }
    return true;
}

// One line per instruction, then its data cycles; its operand words only
// when verbose.
void PinsDcj11::printBacktrace() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    uint32_t from = 0, to = 0;  // the last instruction's bytes
    for (auto i = 0u; i < cycles; ++i) {
        const auto s = g->next(i);
        const uint16_t addr = s->addr16();
        const auto outside = addr < from || addr >= to;
        if (s->fetch() && outside && !discarded(i)) {
            if (Debugger.verbose())
                s->print();
            from = addr;
            to = _mems->disassemble(addr, 1);
        } else if (!s->read() || (outside && !s->fetch()) ||
                   Debugger.verbose()) {
            s->print();
        }
        idle();
    }
}

}  // namespace dcj11
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
