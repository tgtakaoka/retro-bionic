#include "pins_i8086.h"
#include "qs_decode_i8086.h"
#include "debugger.h"
#include "devs_d8087.h"
#include "devs_i8086.h"
#include "digital_bus.h"
#include "inst_i8086.h"
#include "mems_i8086.h"
#include "regs_i8086.h"
#include "signals_i8086.h"

namespace debugger {
namespace i8086 {

// clang-format off
/**
 * 8086 maximum mode bus cycle (8086 datasheet, Figure 15), as the D8087
 * and P8086 boards run it.
 *
 * CLK free-runs, a third of each period high. The Teensy acts as the
 * 8288: a cycle starts when #S2-#S0 leave passive in T4, and READY, held
 * low, keeps it in wait states until the Teensy has served it.
 *
 *         |  T4   |     T1      |     T2      |   T3/Tw     |     T4      |
 *         \_______      \_______      \_______      \_______      \______
 * CLK             /       |      /       |      /       |      /       |
 *         _______  ____________________________________  _______________
 * #S2-#S0 _______><____________status____________________>  passive
 *                   __________    _______________________________________
 * A16-19  ---------X___addr___>--<_______S3-S6 status___________________>
 *                   __________                ____________________
 * AD      ---------X___addr___>--------------<_____read data______>------
 *         _____________________________________________   _______________
 * READY                                                \_/  served, then low
 *
 * Status goes active 110ns at most after T4's rising edge and the address
 * 110ns after T1's falling edge; it holds until T2's. The CPU and the
 * 8087 sample READY on a rising edge (setup 118ns, hold 30ns), and take
 * read data on the falling edge after: T4. Status goes passive 110ns
 * after READY rises.
 *
 * The 8087 takes the bus with #RQ/#GT0 between cycles, and its cycles
 * look the same, S6 high. Without one, #TEST is driven low.
 */
// clang-format on

namespace {

// The bus timing a board's parts take (8086 and 8087 datasheets, NEC
// V-Series Data Book).
struct Timing {
    uint16_t high_min_ns;   // CLK high (TCHCL)
    uint16_t addr_ns;       // address after T1's falling edge (TCLAV)
    uint16_t ready_hold_ns;   // TCHRYX
    uint16_t ready_setup_ns;  // TRYHCH
};
// The CMOS 8 MHz parts: the 80C86A-2/80C88A-2 take CLK high 44ns and low
// 68ns at least, READY 68ns before a rising edge and 20ns after; the
// V20/V30 take READY active by 8ns after the falling edge before the
// rising one. The D8087 board's 8087-1, a 10 MHz part, takes less.
constexpr Timing cmos{44, 70, 20, 68};
const Timing *timing = &cmos;
// The 8087 and the V20/V30 need 2 MHz at least (500ns).
constexpr uint32_t clock_min_hz = 2000000;
// The counter read and the GPIO write between deciding and READY moving.
constexpr auto ready_pad_ns = 20;
// The read data is held past T4's falling edge (TCLDX 10ns).
constexpr auto data_hold_ns = 30;
// Write data and S3-S6 follow T2's falling edge by 110ns at most (60ns
// on the V20/V30).
constexpr auto t2_delay_ns = 130;
// Clocks with no bus cycle before the CPU counts as halted, at the
// slowest rate.
constexpr uint32_t no_status_cycles = 1024 * uint64_t(F_CPU) / clock_min_hz;

const uint8_t PINS_LOW[] = {
        PIN_NMI,
        PIN_INTR,
        PIN_READY,  // every cycle waits
        PIN_MNMX,   // maximum mode
};

const uint8_t PINS_HIGH[] = {
        PIN_RESET,
};

const uint8_t PINS_INPUT[] = {
        PIN_QS0,
        PIN_QS1,
        PIN_RD,
        PIN_LOCK,
        PIN_BHE,
        PIN_INT,
        PIN_RQGT0,
        PIN_RQGT1,
        PIN_A16S,
        PIN_A17S,
        PIN_A18S,
        PIN_A19S,
        PIN_AD0,
        PIN_AD1,
        PIN_AD2,
        PIN_AD3,
        PIN_AD4,
        PIN_AD5,
        PIN_AD6,
        PIN_AD7,
        PIN_AD8,
        PIN_AD9,
        PIN_AD10,
        PIN_AD11,
        PIN_AD12,
        PIN_AD13,
        PIN_AD14,
        PIN_AD15,
};

// The status floats while the bus changes hands; pulled up, it reads
// passive.
const uint8_t PINS_PULLUP[] = {
        PIN_S0,
        PIN_S1,
        PIN_S2,
};

using Clock = PwmClock<PIN_CLK>;

// Counts of the PWM's period at which the edges and windows fall.
uint16_t fall_count;
// T1's address: this many period starts after the status, then this
// count.
uint8_t addr_periods;
uint16_t addr_count;
uint16_t ready_min;
uint16_t ready_max;
uint16_t hold_count;
// T2's data: this many period starts after T1's address, then this count.
uint8_t t2_periods;
uint16_t t2_count;

uint16_t counts(uint32_t ns) {
    return (uint64_t(ns) * F_BUS_ACTUAL + 999999999) / 1000000000;
}

uint16_t period_counts;
// The PWM counts at the bus clock; the CPU's cycle counter at its own.
constexpr uint32_t cycles_per_count = F_CPU / 150000000;

// CLK is high 40% of a period: a longer high half, for a wider READY
// window, failed on the bench.
uint16_t high_counts(uint16_t period) {
    return (uint32_t(period) * 102) >> 8;
}

// READY rises in the high half, past the hold after the rising edge: the
// 8086 takes it at the next rising edge, the V20/V30 at the falling one.
// |ready_max| is the last count to decide at.
void ready_window(uint16_t period, uint16_t fall, uint16_t &min, int &max) {
    min = counts(timing->ready_hold_ns);
    const int setup = period - counts(timing->ready_setup_ns);
    max = (fall < setup ? fall : setup) - counts(ready_pad_ns);
}

// READY has a window in each period to rise in.
bool ready_fits(uint32_t hz) {
    const uint16_t period = F_BUS_ACTUAL / hz;
    uint16_t min;
    int max;
    ready_window(period, high_counts(period), min, max);
    return int(min) <= max;
}

void set_phase();

void set_counts() {
    const uint16_t period = Clock::sm().VAL1 + 1;
    period_counts = period;
    fall_count = high_counts(period);
    Clock::setEdges(0, fall_count);
    const uint16_t addr = fall_count + counts(timing->addr_ns);
    addr_periods = addr / period;
    addr_count = addr % period;
    int max;
    ready_window(period, fall_count, ready_min, max);
    ready_max = max;
    hold_count = fall_count + counts(data_hold_ns);
    const uint16_t t2 = fall_count + counts(t2_delay_ns);
    t2_periods = 1 + t2 / period;
    t2_count = t2 % period;
    set_phase();
}

// The PWM counts the bus clock and the CPU's cycle counter its own, both
// from one PLL: the phase follows from the cycle counter alone, once the
// PWM's counter, a slow peripheral read, has been read when the rate is
// set. |phase_base| is a cycle count at which the PWM counted 0.
uint32_t phase_base;
uint32_t period_cycles;
// Counts the phase is moved by: the PWM counter read lands late by a
// constant the parts take differently.
int phase_offset;

// Reset, and telling the parts apart, run at a rate and phase every part
// takes; then each part runs at its own.
constexpr uint32_t reset_hz = 2500000;
constexpr int reset_phase = -2;
struct ChipClock {
    uint32_t hz;
    int8_t phase;
};
// 8086, 8088, V30, V20, as vSeries() and bus8() index them; measured on
// the P8086 board: the 8086s stop above 5 MHz, and -3 counts is inside
// every part's phase window there.
constexpr ChipClock chip_clocks[4] = {
        {5000000, -3},
        {5000000, -3},
        {5000000, -3},
        {5000000, -3},
};
// K's rate and phase, over the part's own until K 99.
uint32_t override_hz;
bool override_phase_set;
int override_phase;

// The counter read's latency varies; the quickest of many reads lands
// nearest its start, and leaves a constant for |phase_offset| to take up.
void set_phase() {
    // The rate reloads at a period start: let it.
    delayNanoseconds(2 * 1000000000 / clock_min_hz);
    period_cycles = period_counts * cycles_per_count;
    uint32_t best = UINT32_MAX;
    noInterrupts();
    for (auto i = 0; i < 32; ++i) {
        const auto t = ARM_DWT_CYCCNT;
        const auto c = Clock::count();
        const auto took = ARM_DWT_CYCCNT - t;
        if (took < best) {
            best = took;
            phase_base = t - (c + phase_offset) * cycles_per_count;
        }
    }
    interrupts();
}

// Cycles into the PWM's period at |now|; the base moves along by whole
// periods long before the cycle counter wraps.
inline uint32_t phase_at(uint32_t now) {
    if (period_cycles == 0)
        return 0;  // no rate set yet
    auto d = now - phase_base;
    if (d >= (1U << 30)) {
        phase_base += d - d % period_cycles;
        d = now - phase_base;
    }
    return d % period_cycles;
}

// Waits from |now| for |periods| period starts, then for count |count|.
inline void wait_from(uint32_t now, uint8_t periods, uint16_t count) {
    const int ph = phase_at(now);
    const int n = periods ? periods * int(period_cycles) - ph +
                                    count * int(cycles_per_count)
                          : count * int(cycles_per_count) - ph;
    if (n > 0) {
        while (ARM_DWT_CYCCNT - now < uint32_t(n))
            ;
    }
}

// Waits for |periods| period starts, then for count |count|.
inline void wait_count(uint8_t periods, uint16_t count) {
    wait_from(ARM_DWT_CYCCNT, periods, count);
}

// READY rises in its window; interrupts masked.
void raise_ready() {
    const auto now = ARM_DWT_CYCCNT;
    const auto c = phase_at(now) / cycles_per_count;
    if (c < ready_min || c > ready_max)
        wait_from(now, c > ready_max ? 1 : 0, ready_min);
    digitalWriteFast(PIN_READY, HIGH);
}

// READY falls once the CPU has shown it taken, two clocks at least before
// the next cycle samples it: any time will do.
inline void lower_ready() {
    digitalWriteFast(PIN_READY, LOW);
}

void negate_reset() {
    digitalWriteFast(PIN_RESET, LOW);
}

// #S2-#S0 as a number, from the ports as read.
inline uint8_t status_of(uint32_t p7, uint32_t p9) {
    const auto s01 = busValue(S01, p7);  // #S1, #S0
    const auto s2 = (busValue(CNTL, p9) & 0x10) != 0;
    return (s2 ? 4 : 0) | ((s01 & 1) ? 2 : 0) | ((s01 & 2) ? 1 : 0);
}

// #S2-#S0 as a number.
inline uint8_t read_status() {
    const auto s01 = busRead(S01);  // #S1, #S0
    const auto s2 = (busRead(CNTL) & 0x10) != 0;
    return (s2 ? 4 : 0) | ((s01 & 1) ? 2 : 0) | ((s01 & 2) ? 1 : 0);
}

inline bool status_passive() {
    return busRead(S01) == 3 && (busRead(CNTL) & 0x10) != 0;
}

// The halt switch polls USB through yield(), longer than a cycle's
// service; once in this many Teensy cycles is soon enough.
constexpr uint32_t halt_poll_cycles = F_CPU / 100000;
// Bus cycles allowed for an injected sequence and for a step.
constexpr auto exec_cycles = 512;
constexpr auto step_cycles = 1024;

// NMI takes a rising edge, latched; negate once its vector was read.
void assert_nmi() {
    digitalWriteFast(PIN_NMI, HIGH);
}

void negate_nmi() {
    digitalWriteFast(PIN_NMI, LOW);
}

constexpr uint32_t PARK =
        InstI8086::physical(InstI8086::PARK_CS, InstI8086::PARK_IP);

// The PARK vector's byte at |addr|, IP then CS.
uint8_t parkVector(uint32_t addr) {
    switch (addr & 3) {
    case 0:
        return uint8_t(InstI8086::PARK_IP);
    case 1:
        return uint8_t(InstI8086::PARK_IP >> 8);
    case 2:
        return uint8_t(InstI8086::PARK_CS);
    default:
        return uint8_t(InstI8086::PARK_CS >> 8);
    }
}

bool isVector(const Signals *s) {
    return s->code() && InstI8086::vector(s->addr);
}

// A fetch the queue makes from PARK on.
bool atPark(const Signals *s) {
    return s->code() && ((s->addr - PARK) & 0xFFFFF) < 16;
}

}  // namespace

PinsI8086::PinsI8086(bool fpu)
    : _fpu(fpu), _clock(cmos.high_min_ns, clock_min_hz) {
    _devs = fpu ? new DevsD8087() : new DevsI8080();
    _mems = new MemsI8086(fpu);
    _regs = new RegsI8086(this);
#ifdef TUNE_CLOCK
    _clockInput = reset_hz;
#endif
}

#ifdef TUNE_CLOCK
bool PinsI8086::setClockInput(uint32_t hz) {
    // K 16-31 move the phase by -8 to +7 counts (24 is none), K 99 goes
    // back to the part's own rate and phase; each prints them.
    // ATTENTION: K above 6 MHz on the 80C86A-2 wedged the firmware until
    // a power cycle; the watchdog did not fire. Not found yet.
    if (hz < 100000) {
        const auto n = hz / 1000;
        if (n >= 16 && n < 32) {
            override_phase_set = true;
            override_phase = phase_offset = n - 24;
            set_counts();
        } else if (n == 91) {  // DIAG: the ring raw, for a host replay
            dumpQueue();
            return true;
        } else if (n == 93) {  // DIAG: the backtrace again, and its QS check
            if (_qsMode == QS_RAW || !_qs.frozen() || !printExecuted(true))
                printCycles();
            return true;
        } else if (n == 94) {  // DIAG: backtrace by QS pairs, QS0, or raw
            _qsMode = _qsMode == QS_PAIR ? QS_ZERO
                    : _qsMode == QS_ZERO ? QS_RAW : QS_PAIR;
            cli.println(_qsMode == QS_PAIR   ? "QS pair backtrace"
                        : _qsMode == QS_ZERO ? "QS0 backtrace"
                                             : "raw backtrace");
            return true;
        } else if (n == 99) {
            override_hz = 0;
            override_phase_set = false;
            applyChipClock();
        } else {
            return false;
        }
        cli.print("phase=");
        cli.printDec(phase_offset + 24);
        cli.print(override_phase_set ? "(K)" : "");
        cli.print(" clock=");
        cli.printDec(_clockInput);
        cli.println(override_hz ? "(K)" : "");
        return true;
    }
    if (hz == 0 || !ready_fits(hz))
        return false;
    // The rate reloads at a period start, so the running clock never
    // glitches.
    const auto rate = _clock.setRate(hz);
    if (rate == 0)
        return false;
    override_hz = hz;
    _clockInput = rate;
    set_counts();
    return true;
}
#endif

// Reset starts at FFFF0H; the CPU waits in that fetch, the registers are
// read there.
void PinsI8086::resetPins() {
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
    pinsMode(PINS_PULLUP, sizeof(PINS_PULLUP), INPUT_PULLUP);
    // The 8087's BUSY, or low so WAIT falls through.
    if (_fpu) {
        pinMode(PIN_TEST, INPUT);
    } else {
        pinMode(PIN_TEST, OUTPUT);
        digitalWriteFast(PIN_TEST, LOW);
    }
    // RESET is held high: a glitch on CLK is harmless.
    _clock.begin(reset_hz);
    _clock.start();
    _qs.begin();
    phase_offset = reset_phase;
    set_counts();

    // RESET must be high 4 clocks at least, MN/#MX set before.
    delayNanoseconds(16 * (1000000000 / clock_min_hz));
    Cycles::reset();
    _latched = false;
    negate_reset();
    // Read as an 8086's; the 8088 has no #BHE, which stays high.
    _bus8 = false;
    auto s = prepareCycle();
    const auto bus8 = s && !s->bhe();
    if (bus8) {
        s->setBus8(true);
        s->setStatus(read_status());  // still in its wait states
    }
    if (parkAtReset(s, bus8))
        applyChipClock();
    // The first reset after power-up can tell the parts apart wrongly; the
    // second one never has.
    static bool once;
    if (!once) {
        once = true;
        resetPins();
    }
}

// The part's own rate and phase, or K's; the CPU waits parked meanwhile.
void PinsI8086::applyChipClock() {
    const auto &chip = chip_clocks[(_v_series ? 2 : 0) + (_bus8 ? 1 : 0)];
    const auto hz = override_hz ? override_hz : chip.hz;
    if (!ready_fits(hz))
        return;
    const auto rate = _clock.setRate(hz);
    if (rate == 0)
        return;
#ifdef TUNE_CLOCK
    _clockInput = rate;
#endif
    phase_offset = override_phase_set ? override_phase : chip.phase;
    set_counts();
}

// Waits for the next bus cycle and holds it in wait states, its address
// and status read. Null when the CPU shows none: halted.
Signals *PinsI8086::prepareCycle() {
    if (!_latched) {
        noInterrupts();
        _latched = latchCycle();
        interrupts();
        if (!_latched)
            return nullptr;
    }
    _latched = false;
    auto s = Signals::put();
    *s = _latch;
    s->clear();  // neither injected nor captured yet
    _seenAt[s->pos()] = _latchSeen;
    // A fetch goes through CS, and a read has no data to take: no need for
    // T2, unless S3-S6 tell a vector read, or the 8087's own cycle (S6).
    if (!_fpu && (_latchStatus == Signals::STATUS_CODE ||
                         (_latchStatus == Signals::STATUS_READ &&
                                 !InstI8086::vector(s->addr)))) {
        s->setSegment(_latchStatus == Signals::STATUS_CODE ? SEG_CS : SEG_DS);
        return s;
    }
    // On to T2's write data and S3-S6; late is harmless, as the CPU waits
    // in T3 for READY.
    noInterrupts();
    wait_count(t2_periods, t2_count);
    interrupts();
    s->getSegment();
    return s;
}

// Waits, interrupts masked, for the next cycle's status and reads its T1
// address into _latch, not the ring: while the ring is held, the head is
// still the cycle the caller is finishing. False when the CPU shows none.
bool PinsI8086::latchCycle() {
    const auto start = ARM_DWT_CYCCNT;
    while (status_passive()) {
        if (ARM_DWT_CYCCNT - start >= no_status_cycles)
            return false;
    }
    // T4, past its rising edge. On to T1's address, or T1's high half.
    const auto seen = ARM_DWT_CYCCNT;
    _latchSeen = seen;
    wait_from(seen, addr_periods, addr_count);
    // One read of each port: the address, #BHE and the status.
    const uint32_t p6 = PIN(AD);
    const uint32_t p7 = PIN(AH);
    const uint32_t p9 = PIN(CNTL);
    _latch.setAddr(_bus8, busValue(AH, p7) | busValue(AD, p6),
            busValue(CNTL, p9));
    _latchStatus = status_of(p7, p9);
    _latch.setStatus(_latchStatus);
    return true;
}

// Takes up a cycle waiting at |addr|, the fetch every park is.
Signals *PinsI8086::resumeCycle(uint32_t addr) {
    auto s = Signals::put();
    s->addr = addr;
    s->setFetch(_bus8);
    s->setStatus(Signals::STATUS_CODE);
    return s;
}

Signals *PinsI8086::completeCycle(Signals *s) {
    const auto drive = s->read() || s->intAck();
    if (drive) {
        if (s->readMemory())
            retrieveData(s);
        s->outData();
    } else if (s->write()) {
        s->getData();
        if (s->writeMemory())
            storeData(s);
    }
    // Masked: the read data must stay through T4's falling edge and go
    // before T1's. READY rises in a high half: the 8086 takes it at the
    // next rising edge, the V20/V30 at the falling edge before; either
    // way T3 ends, and T4 starts, at the falling edge after the next
    // period start.
    noInterrupts();
    raise_ready();
    wait_count(1, hold_count);
    if (drive)
        s->inputMode();
    lower_ready();
    Cycles::next();
    // Still masked: the next T1 follows within a clock, and an interrupt
    // taken now would miss its address. READY then holds that cycle.
    _latched = latchCycle();
    interrupts();
    return s;
}

void PinsI8086::Overlay::put(uint32_t addr, uint8_t data) {
    auto i = find(addr);
    if (i < 0) {
        if (_num >= MAX)
            return;
        i = _num++;
        _addr[i] = addr;
    }
    _data[i] = data;
    _read[i] = false;
}

int PinsI8086::Overlay::find(uint32_t addr) const {
    for (uint_fast8_t i = 0; i < _num; ++i) {
        if (_addr[i] == addr)
            return i;
    }
    return -1;
}

bool PinsI8086::Overlay::get(uint32_t addr, uint8_t &data) const {
    const auto i = find(addr);
    if (i < 0)
        return false;
    data = _data[i];
    return true;
}

uint16_t PinsI8086::Overlay::wordAt(uint32_t addr) const {
    uint8_t l = 0, h = 0;
    get(addr, l);
    get((addr + 1) & 0xFFFFF, h);
    return uint16(h, l);
}

void PinsI8086::Overlay::markRead(uint32_t addr) {
    const auto i = find(addr);
    if (i >= 0)
        _read[i] = true;
}

bool PinsI8086::Overlay::allRead() const {
    for (uint_fast8_t i = 0; i < _num; ++i) {
        if (!_read[i])
            return false;
    }
    return true;
}

bool PinsI8086::parkAtReset(Signals *s, bool bus8) {
    const auto reset =
            InstI8086::physical(InstI8086::RESET_CS, InstI8086::RESET_IP);
    _bus8 = bus8;
    if (s)
        s->setBus8(bus8);
    if (s == nullptr || !s->read() || !s->memory() || s->addr != reset) {
        cli.print("?reset: ");
        if (s)
            s->print();
        else
            cli.println();
        return false;
    }
    auto regs = this->regs<RegsI8086>();
    regs->parkAt(InstI8086::RESET_CS, InstI8086::RESET_IP);
    regs->save();
    _v_series = regs->probeVSeries();
    return true;
}

// A read's data from memory, a device, or an acknowledge's vector.
void PinsI8086::retrieveData(Signals *s) {
    if (s->intAck()) {
        s->data = _devs->vector();
    } else if (s->memory()) {
        if (_bus8) {
            s->data = _mems->read_byte(s->addr);
        } else {
            // Both lanes: the CPU takes the one #BHE and A0 pick.
            s->data = _mems->read16(s->addr & ~1);
        }
    } else if (_devs->isSelected(s->addr)) {
        const uint16_t v = _devs->read(s->addr);
        s->data = (_bus8 || s->wordAccess()) ? v : (v | (v << 8));
    } else {
        s->data = 0xFFFF;
    }
}

// A write's data to memory or a device.
void PinsI8086::storeData(Signals *s) {
    if (s->memory()) {
        if (s->wordAccess()) {
            _mems->write16(s->addr, s->data);
        } else {
            _mems->write_byte(s->addr, s->byteAt(s->addr));
        }
    } else if (_devs->isSelected(s->addr)) {
        _devs->write(s->addr, s->wordAccess() ? s->data : s->byteAt(s->addr));
    }
}

void PinsI8086::injectRead(
        Signals *s, const uint8_t *inst, uint_fast8_t len, uint32_t org) {
    auto injected = false;
    auto byteAt = [&](uint32_t a) -> uint8_t {
        const uint32_t off = (a - org) & 0xFFFFF;
        if (off < len) {
            injected = true;
            return inst[off];
        }
        uint8_t b;
        if (_overlay.get(a, b)) {
            injected = true;
            _overlay.markRead(a);
            return b;
        }
        return _mems->read_byte(a);
    };
    uint16_t data;
    if (s->wordAccess()) {
        const auto l = byteAt(s->addr);
        data = uint16(byteAt((s->addr + 1) & 0xFFFFF), l);
    } else {
        const auto b = byteAt(s->addr);
        data = _bus8 ? b : uint16(b, b);
    }
    // Memory alone is left to retrieveData().
    if (injected)
        s->inject(data);
}

void PinsI8086::execInst(
        const uint8_t *inst, uint_fast8_t len, uint32_t &org, uint32_t exit) {
    const uint32_t last = (org + len - 1) & 0xFFFFF;
    const auto start = org;
    auto whole = false;
    auto s = resumeCycle(org);
    for (auto guard = exec_cycles; s && guard; --guard) {
        if (whole && (exit == EXIT_OVERLAY ? _overlay.allRead()
                                           : (s->code() && s->addr == exit)))
            break;
        if (s->coproc()) {
            // The 8087's own operand: memory's, as when running.
        } else if (s->memory() && s->read()) {
            injectRead(s, inst, len, start);
            if (s->covers(last))
                whole = true;
        } else if (s->write()) {
            s->capture();
        }
        completeCycle(s);
        if (s->write() && s->memory() && !s->coproc()) {
            for (uint_fast8_t i = 0; i < 2; ++i) {
                const auto a = (s->addr + i) & 0xFFFFF;
                if (s->covers(a))
                    _overlay.put(a, s->byteAt(a));
            }
        }
        s = prepareCycle();
    }
    _next = s;
    if (s)
        org = s->addr;
}

// Keeps the last 6 bytes written: a trap's frame, once it parks.
void PinsI8086::recordFrame(const Signals *s) {
    for (uint_fast8_t i = 0; i < 2; ++i) {
        const auto a = (s->addr + i) & 0xFFFFF;
        if (!s->covers(a))
            continue;
        for (uint_fast8_t j = 1; j < 6; ++j) {
            _frameAt[j - 1] = _frameAt[j];
            _frame[j - 1] = _frame[j];
        }
        _frameAt[5] = a;
        _frame[5] = s->byteAt(a);
        if (_frameBytes < 6)
            _frameBytes++;
    }
}

// |s| is the fetch at PARK after a trap's vector: the frame below holds
// the program's IP, CS and FLAGS.
bool PinsI8086::parkInTrap(Signals *s) {
    if (_frameBytes < 6)
        return false;
    uint32_t base = _frameAt[0];
    for (uint_fast8_t i = 1; i < 6; ++i) {
        if (((_frameAt[i] - base) & 0xFFFFF) > 0x80000)
            base = _frameAt[i];
    }
    uint8_t bytes[6];
    for (uint_fast8_t k = 0; k < 6; ++k) {
        const auto a = (base + k) & 0xFFFFF;
        auto found = false;
        for (uint_fast8_t i = 0; i < 6; ++i) {
            if (_frameAt[i] == a) {
                bytes[k] = _frame[i];
                found = true;
            }
        }
        if (!found)
            return false;
    }
    const auto ip = uint16(bytes[1], bytes[0]);
    const auto cs = uint16(bytes[3], bytes[2]);
    const auto flags = uint16(bytes[5], bytes[4]);
    // Drop the debugger's part: the pushes and the vector reads.
    auto from = s;
    for (uint_fast8_t i = 1; i <= 16 && i <= Signals::get()->diff(s); ++i) {
        const auto t = s->prev(i);
        if (isVector(t) || (atPark(t) && t != s) ||
                (t->write() && t->memory() && ((t->addr - base) & 0xFFFFF) < 6))
            from = t;
    }
    Cycles::discard(from);
    auto regs = this->regs<RegsI8086>();
    const uint16_t parkIp = InstI8086::PARK_IP + ((s->addr - PARK) & 0xFFFFF);
    regs->parkInTrap(cs, ip, flags, _vector, parkIp);
    return true;
}

// Completes cycles from |s| until the CPU takes one of the debugger's
// vectors and fetches at PARK. A halted CPU, or the halt switch while
// running, gets NMI.
bool PinsI8086::untilTrap(Signals *s, Mode mode) {
    _vector = 0;
    _frameBytes = 0;
    auto nmi = false;
    auto polled = ARM_DWT_CYCCNT;
    uint32_t guard = mode == STEP ? step_cycles : UINT32_MAX;
    for (uint32_t n = 0; guard; ++n, --guard) {
        if (s == nullptr) {
            if (nmi)
                break;
            assert_nmi();
            nmi = true;
            s = prepareCycle();
            continue;
        }
        if (isVector(s)) {
            // The frame is what the trap pushes from here on.
            if (_vector == 0)
                _frameBytes = 0;
            _vector = s->addr >> 2;
            const auto b = parkVector(s->addr);
            if (s->wordAccess()) {
                s->inject(uint16(parkVector(s->addr + 1), b));
            } else {
                s->inject(_bus8 ? b : uint16(b, b));
            }
            if (_vector == InstI8086::VEC_NMI)
                negate_nmi();
        } else if (_vector && atPark(s)) {
            if (_frameBytes == 6) {
                negate_nmi();
                return parkInTrap(s);
            }
            // The CPU fetches at PARK before it pushes IP; the queue gets
            // NOPs until the frame is whole, and it parks at the next fetch.
            s->inject(uint16(InstI8086::NOP, InstI8086::NOP));
        } else if (s->halt() && !nmi) {
            assert_nmi();
            nmi = true;
        }
        completeCycle(s);
        if (s->write() && s->memory() && !s->coproc())
            recordFrame(s);
        if (mode == RUN) {
            _devs->loop();
            if (ARM_DWT_CYCCNT - polled >= halt_poll_cycles && !nmi &&
                    (polled = ARM_DWT_CYCCNT, haltSwitch())) {
                assert_nmi();
                nmi = true;
            }
        }
        s = prepareCycle();
    }
    negate_nmi();
    return false;
}

// Stops the ring early in a CLK period, before that period's sample: the
// latest sample is then the previous rising edge's, at a known time. The
// rising edge is the PWM's count 0.
void PinsI8086::freezeQs() {
    const uint32_t sample = _qs.delay() * cycles_per_count;
    const uint32_t off = (period_cycles - (phase_offset * int(cycles_per_count)) %
                                                  int(period_cycles)) %
                         period_cycles;
    noInterrupts();
    auto now = ARM_DWT_CYCCNT;
    auto ph = (phase_at(now) + off) % period_cycles;
    // Bounded: one period passes in a few hundred cycles.
    for (auto i = 0; i < 1000 && ph >= sample / 2; ++i) {
        now = ARM_DWT_CYCCNT;
        ph = (phase_at(now) + off) % period_cycles;
    }
    _qs.freeze(now - ph - period_cycles + sample);
    interrupts();
}

// DIAG: the ring raw, for a host replay of the decoder: each cycle's time
// before the latest sample, in CPU cycles, and the cycle; then the queue
// status as far back as the oldest cycle, oldest first.
void PinsI8086::dumpQueue() {
    const auto g = Signals::get();
    const int n = g->diff(Signals::put());
    cli.print("QSDUMP period=");
    cli.printDec(period_cycles);
    cli.print(" delay=");
    cli.printDec(_qs.delay());
    cli.print(" cycles=");
    cli.printDec(n);
    cli.println();
    for (int i = 0; i < n; ++i) {
        const auto s = g->next(i);
        cli.printDec(_qs.stopCycle() - _seenAt[s->pos()]);
        cli.print(' ');
        s->print();
    }
    const int first =
            n ? (_qs.stopCycle() - _seenAt[g->pos()]) / period_cycles : 0;
    cli.print("QS");
    for (int b = first < int(QsTracker::SAMPLES) ? first : QsTracker::SAMPLES - 1,
             k = 0;
            b >= 0; --b, ++k) {
        if (k % 64 == 0)
            cli.println();
        cli.print(char('0' + _qs.sampleAt(b)));
    }
    cli.println();
    cli.println("QSEND");
}

// At the prompt too, the phase base keeps up with the cycle counter.
void PinsI8086::idle() {
    phase_at(ARM_DWT_CYCCNT);
}

bool PinsI8086::step(bool show) {
    // A HLT waits for an interrupt; stepping into it would hang.
    auto regs = this->regs<RegsI8086>();
    if (_mems->read(regs->nextIp()) == InstI8086::HLT)
        return false;
    Cycles::reset();
    _qs.thaw();
    {
        Cycles::Hold hold;
        regs->restore(true);
    }
    const auto trapped = untilTrap(_next, STEP);
    freezeQs();
    if (trapped) {
        if (show)
            printCycles();
        Cycles::Hold hold;
        regs->save();
        return true;
    }
    return false;
}

void PinsI8086::run() {
    Cycles::reset();
    {
        Cycles::Hold hold;
        _regs->restore();
    }
    saveBreakInsts();
    _qs.thaw();
    startRunTimer();
    const auto stopped = untilTrap(_next, RUN);
    stopRunTimer();
    freezeQs();
    restoreBreakInsts();
    if (stopped) {
        Cycles::Hold hold;
        _regs->save();
    }
    disassembleCycles();
}

void PinsI8086::setBreakInst(uint32_t addr) const {
    _mems->put_prog(addr, InstI8086::INT3);
}

// Type 2 raises NMI, an edge the CPU latches and the trap negates; any
// other type raises INTR, high while a device's vector() is pending.
void PinsI8086::assertInt(uint8_t name) {
    if (name == InstI8086::VEC_NMI) {
        assert_nmi();
    } else {
        digitalWriteFast(PIN_INTR, HIGH);
    }
}

void PinsI8086::negateInt(uint8_t name) {
    if (name != InstI8086::VEC_NMI)
        digitalWriteFast(PIN_INTR, _devs->vector() ? HIGH : LOW);
}

void PinsI8086::printCycles() {
    const auto g = Signals::get();
    const auto cycles = g->diff(Signals::put());
    for (auto i = 0u; i < cycles; ++i) {
        g->next(i)->print();
        idle();
    }
}

// No status tells a fetch from a prefetch, so the backtrace prints every
// cycle.
const SignalsImpl *PinsI8086::findBacktraceStart() {
    return Cycles::tail();
}

void PinsI8086::printBacktrace() {
    if (_qsMode != QS_RAW && _qs.frozen() && printExecuted(false))
        return;
    printCycles();
}

namespace {

// The ring and the frozen queue status, as QsDecoder reads them.
struct QsSource {
    const Signals *g;
    int n;
    const uint32_t *seenAt;
    const QsTracker &tracker;
    uint32_t period;
    int first;  // the samples back from the stop that the oldest cycle has
    const MemsI8086 *mems;

    int cycles() const { return n; }
    uint32_t cycleTime(int i) const { return seenAt[g->next(i)->pos()]; }
    bool fetch(int i, uint32_t &first, uint32_t &next) const {
        const auto s = g->next(i);
        if (!s->code() || InstI8086::vector(s->addr))
            return false;
        first = s->covers(s->addr) ? s->addr : (s->addr + 1) & 0xFFFFF;
        next = (s->addr + (s->wordAccess() ? 2 : 1)) & 0xFFFFF;
        return true;
    }
    int samples() const { return first + 1; }
    uint32_t sampleTime(int k) const {
        return tracker.stopCycle() - uint32_t(first - k) * period;
    }
    uint8_t qs(int k) const { return tracker.sampleAt(first - k); }
    uint32_t insn(uint32_t addr) const { return mems->disassemble(addr, 1); }
    uint8_t length(uint32_t addr) const { return mems->insnLength(addr); }
    void cycle(int i) const { g->next(i)->print(); }
};

}  // namespace

// The instructions the CPU executed, from the frozen queue status. False
// when the ring holds no instruction to show.
bool PinsI8086::printExecuted(bool stats) {
    const auto g = Signals::get();
    const int n = g->diff(Signals::put());
    if (n == 0)
        return false;
    const int first = (_qs.stopCycle() - _seenAt[g->pos()]) / period_cycles;
    if (first >= int(QsTracker::SAMPLES))
        return false;
    QsSource src{g, n, _seenAt, _qs, period_cycles, first,
            static_cast<const MemsI8086 *>(_mems)};
    const auto pairs = _qsMode == QS_PAIR;
    QsDecoder<QsSource, Cycles::MAX_CYCLES * 2> dec(
            src, pairs, this->regs<RegsI8086>()->nextIp());
    if (!dec.decode(_lineLimit))
        return false;
    if (pairs && stats) {
        cli.print("QS starts good=");
        cli.printDec(dec.good);
        cli.print(" bad=");
        cli.printDec(dec.bad);
        cli.print(" delay=");
        cli.printDec(_qs.delay());
        cli.println();
    }
    return true;
}

}  // namespace i8086
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
