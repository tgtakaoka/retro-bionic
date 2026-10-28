#include "pins_d8087.h"
#include "debugger.h"
#include "devs_d8087.h"
#include "digital_bus.h"
#include "inst_i8086.h"
#include "regs_i8086.h"
#include "signals_i8086.h"

namespace debugger {
namespace i8086 {

// clang-format off
/**
 * 8086 maximum mode bus cycle (8086 datasheet, Figure 15), as the D8087
 * board runs it.
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
 * look the same, S6 high.
 */
// clang-format on

namespace {

// The 8087 takes 2-5 MHz (TCLCL 200-500ns), high 69ns at least; CLK is
// high a third of each period, as from an 8284.
constexpr uint32_t clock_hz = 2500000;
constexpr uint32_t clock_min_hz = 2000000;
constexpr auto clock_high_min_ns = 69;
constexpr uint8_t clock_duty = 85;
// The address follows T1's falling edge by 110ns at most.
constexpr auto addr_delay_ns = 120;
// READY may change only this far from a rising edge: the hold, then the
// setup, padded for the counter read and the GPIO write.
constexpr auto ready_hold_ns = 30 + 20;
constexpr auto ready_setup_ns = 118 + 40;
// The read data is held past T4's falling edge (TCLDX 10ns).
constexpr auto data_hold_ns = 30;
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
        PIN_BUSY,
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
uint16_t addr_count;
uint16_t ready_min;
uint16_t ready_max;
uint16_t hold_count;

uint16_t counts(uint32_t ns) {
    return (uint64_t(ns) * F_BUS_ACTUAL + 999999999) / 1000000000;
}

// READY has a window in each period to change in.
bool ready_fits(uint32_t hz) {
    return counts(ready_hold_ns) + counts(ready_setup_ns) < F_BUS_ACTUAL / hz;
}

void set_counts() {
    const uint16_t period = Clock::sm().VAL1 + 1;
    fall_count = (uint32_t(period) * clock_duty) >> 8;
    addr_count = fall_count + counts(addr_delay_ns);
    ready_min = counts(ready_hold_ns);
    ready_max = period - counts(ready_setup_ns);
    hold_count = fall_count + counts(data_hold_ns);
}

// Waits, interrupts masked, for the count to reach |count| in this period.
inline void until_count(uint16_t count) {
    while (Clock::count() < count)
        ;
}

// Waits, interrupts masked, for the next period to start.
inline void until_period() {
    auto prev = Clock::count();
    while (true) {
        const auto c = Clock::count();
        if (c < prev)
            return;
        prev = c;
    }
}

// READY changes away from the rising edges, where the CPU and the 8087
// sample it; interrupts masked.
void set_ready(bool high) {
    while (true) {
        const auto c = Clock::count();
        if (c >= ready_min && c <= ready_max)
            break;
    }
    digitalWriteFast(PIN_READY, high ? HIGH : LOW);
}

void negate_reset() {
    digitalWriteFast(PIN_RESET, LOW);
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

}  // namespace

PinsD8087::PinsD8087()
    : PinsI8086(new DevsD8087(), true),
      _clock(clock_high_min_ns, clock_min_hz, clock_duty) {
#ifdef TUNE_CLOCK
    _clockInput = clock_hz;
#endif
}

#ifdef TUNE_CLOCK
bool PinsD8087::setClockInput(uint32_t hz) {
    if (hz == 0 || !ready_fits(hz))
        return false;
    // The rate reloads at a period start, so the running clock never
    // glitches.
    const auto rate = _clock.setRate(hz);
    if (rate == 0)
        return false;
    _clockInput = rate;
    set_counts();
    return true;
}
#endif

// Reset starts at FFFF0H; the CPU waits in that fetch, the registers are
// read there.
void PinsD8087::resetPins() {
    pinsMode(PINS_HIGH, sizeof(PINS_HIGH), OUTPUT, HIGH);
    pinsMode(PINS_LOW, sizeof(PINS_LOW), OUTPUT, LOW);
    pinsMode(PINS_INPUT, sizeof(PINS_INPUT), INPUT);
    pinsMode(PINS_PULLUP, sizeof(PINS_PULLUP), INPUT_PULLUP);
#ifdef TUNE_CLOCK
    const auto hz = _clockInput;
#else
    const auto hz = clock_hz;
#endif
    // RESET is held high: a glitch on CLK is harmless.
    _clock.begin(hz);
    _clock.start();
    set_counts();

    // RESET must be high 4 clocks at least, MN/#MX set before.
    delayNanoseconds(16 * (1000000000 / clock_min_hz));
    Cycles::reset();
    negate_reset();
    // Read as an 8086's; the 8088 has no #BHE, which stays high.
    _bus8 = false;
    auto s = prepareCycle();
    const auto bus8 = s && !s->bhe();
    if (bus8) {
        s->setBus8(true);
        s->setStatus(read_status());  // still in its wait states
    }
    parkAtReset(s, bus8);
}

// Waits for the next bus cycle and holds it in wait states, its address
// and status read. Null when the CPU shows none: halted.
Signals *PinsD8087::prepareCycle() {
    const auto limit = no_status_cycles;
    // Masked: the address is there only in T1.
    noInterrupts();
    // The last cycle's status first goes passive.
    auto start = ARM_DWT_CYCCNT;
    while (!status_passive()) {
        if (ARM_DWT_CYCCNT - start >= limit) {
            interrupts();
            return nullptr;
        }
    }
    start = ARM_DWT_CYCCNT;
    while (status_passive()) {
        if (ARM_DWT_CYCCNT - start >= limit) {
            interrupts();
            return nullptr;
        }
    }
    // T4, past its rising edge. On to T1's address, or T1's high half.
    const auto c0 = Clock::count();
    while (true) {
        const auto c = Clock::count();
        if (c < c0 || c >= addr_count)
            break;
    }
    auto s = Signals::put();
    s->getAddr(_bus8);
    s->setStatus(read_status());
    interrupts();
    // S3-S6 from T2 on; the CPU waits in T3 for READY.
    delayNanoseconds(1000000000 / clock_min_hz);
    s->getSegment();
    return s;
}

// Takes up a cycle waiting at |addr|, the fetch every park is.
Signals *PinsD8087::resumeCycle(uint32_t addr) {
    auto s = Signals::put();
    s->addr = addr;
    s->setFetch(_bus8);
    s->setStatus(Signals::STATUS_CODE);
    return s;
}

Signals *PinsD8087::completeCycle(Signals *s) {
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
    // before T1's.
    noInterrupts();
    set_ready(true);
    until_period();  // READY sampled: T4 next
    until_count(hold_count);
    if (drive)
        s->inputMode();
    set_ready(false);
    interrupts();
    Cycles::next();
    return s;
}

}  // namespace i8086
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
