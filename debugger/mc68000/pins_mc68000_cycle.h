#ifndef __PINS_MC68000_CYCLE_H__
#define __PINS_MC68000_CYCLE_H__

// The cycle engine of PinsMc68000Base, included by each board's file to
// instantiate it with its own class, and by the base for the constants.

#include "debugger.h"
#include "devs_mc68000.h"
#include "digital_bus.h"
#include "inst_mc68000.h"
#include "mems_mc68000.h"
#include "pins_mc68000_base.h"
#include "regs_mc68000.h"
#include "signals_mc68000.h"

namespace debugger {
namespace mc68000 {

// No #AS for this many clocks: the CPU stopped in STOP, or halted on a
// double bus fault. Long enough to clear the slowest instruction.
constexpr auto halt_clocks = 1024;
// Clocks between polls of a CPU already stopped: short enough to service
// the devices and the halt switch, and to tick the watchdog, often.
constexpr auto wait_poll_clocks = 16;
// A cycle #DTACK ends negates #AS within a few clocks; this bounds the
// wait, interrupts masked, when it doesn't. Bounds the wait for a
// write's data strobes too.
constexpr auto dtack_hold_clocks = 16;
// The 10 MHz parts take 2-10 MHz with halves of at least 45 ns (M68000UM
// 10.8, 10.9); K trusts the user with the grade.
constexpr uint32_t clock_input_min_hz = 2000000;
constexpr auto clock_half_min_ns = 45;
// Teensy cycles in a clock at the slowest rate: timeouts that hold at any.
constexpr uint32_t clock_cycles = F_CPU / clock_input_min_hz;


inline void assert_dtack() {
    digitalWriteFast(PIN_DTACK, LOW);
}

inline void negate_dtack() {
    digitalWriteFast(PIN_DTACK, HIGH);
}

inline bool memRead(const Signals *s) {
    return s->read() && !s->iack();
}

template <typename BOARD>
Signals *PinsMc68000Base::prepareCycleOf(uint_fast16_t clocks) {
    const auto board = static_cast<const BOARD *>(this);
    auto s = Signals::put();
    s->clearMark();  // Cycles::next() keeps the slot's old mark
    const uint32_t start = ARM_DWT_CYCCNT;
    const uint32_t limit = clocks * clock_cycles;
#ifdef PROFILE_CYCLES
    assert_debug();  // polling for #AS
#endif
    uint8_t cntl;
    while ((cntl = busRead(CNTL)) & Signals::AS) {
        if (ARM_DWT_CYCCNT - start >= limit) {
            s->markHalt();
            return s;
        }
    }
#ifdef PROFILE_CYCLES
    negate_debug();  // #AS seen
#endif
    s->cntl() = cntl;
    board->getAddr(s);
    return s;
}

template <typename BOARD>
Signals *PinsMc68000Base::completeCycleOf(Signals *s) {
    const auto board = static_cast<const BOARD *>(this);
    if (s->halt()) {
        // Nothing moves; not recorded, so s->prev() stays the previous
        // real cycle.
        board->inputMode();
        return s;
    }
    const uint32_t limit = dtack_hold_clocks * clock_cycles;
#ifdef PROFILE_CYCLES
    assert_debug();  // the cycle served
#endif
    if (s->read()) {
        serveRead(s);
#ifdef PROFILE_CYCLES
        negate_debug();  // looked up
#endif
        // The CPU latches read data a clock after it samples #DTACK, and
        // wants it no later than #DTACK plus M68000UM parameter #31.
        noInterrupts();
        assert_dtack();
        board->outData(s);
    } else {
        // ATTENTION: unmeasured. Write data is driven from S3, the data
        // strobes asserted in S4, so the data and its lanes are read as a
        // strobe is first seen low.
        const uint32_t start = ARM_DWT_CYCCNT;
        uint8_t cntl;
        while (((cntl = busRead(CNTL)) & _strobes) == _strobes) {
            if (ARM_DWT_CYCCNT - start >= limit)
                break;
        }
        s->cntl() = cntl;
        board->getLanes(s);
        board->getData(s);
        noInterrupts();
        assert_dtack();
    }
    // Masked from #DTACK on, or an interrupt could outlast #AS high and the
    // next cycle end on this #DTACK. #DTACK is negated about 50 ns after
    // #AS rises, well before the next cycle's S4. The store waits until
    // #DTACK is off: a device or an EXTMEM miss can outlast #AS.
    Cycles::next();
    const uint32_t start = ARM_DWT_CYCCNT;
    uint8_t cntl;
    while (((cntl = busRead(CNTL)) & Signals::AS) == 0) {
        if (ARM_DWT_CYCCNT - start >= limit)
            break;
        // TAS holds #AS from its read through its write (M68000UM Figure
        // 5-9): R/#W falls for the write, a clock before its data.
        if ((cntl & Signals::RW) == 0 && s->read()) {
            negate_dtack();
            board->inputMode();
            s = completeTasWrite<BOARD>(s, cntl);
            break;
        }
    }
    negate_dtack();
#ifdef PROFILE_CYCLES
    assert_debug();  // released
#endif
    interrupts();
    if (s->read()) {
        board->inputMode();  // only a read drove the bus
    } else {
        // Before the next cycle is looked up, so a read of the same
        // address sees it.
        storeWrite(s);
    }
#ifdef PROFILE_CYCLES
    negate_debug();  // back to the run loop
#endif
    return s;
}

// TAS's write, at the address its read left on the bus: served as its
// own cycle, from its strobes to #AS rising.
template <typename BOARD>
Signals *PinsMc68000Base::completeTasWrite(const Signals *r, uint8_t cntl) {
    const auto board = static_cast<const BOARD *>(this);
    const uint32_t limit = dtack_hold_clocks * clock_cycles;
    auto s = Signals::put();
    s->clearMark();
    s->addr = r->addr;
    s->setFc(r->fc(), false);
    const uint32_t start = ARM_DWT_CYCCNT;
    while ((cntl & _strobes) == _strobes) {
        if (ARM_DWT_CYCCNT - start >= limit)
            break;
        cntl = busRead(CNTL);
    }
    s->cntl() = cntl;
    board->getLanes(s);
    board->getData(s);
    assert_dtack();
    Cycles::next();
    while ((busRead(CNTL) & Signals::AS) == 0) {
        if (ARM_DWT_CYCCNT - start >= 2 * limit)
            break;
    }
    return s;
}

// The ring is left holding the program's cycles only.
template <typename BOARD>
bool PinsMc68000Base::loopOf() {
    // ATTENTION: a program that moves VBR in a run hides its ILLEGAL from
    // the debugger until the next stop.
    const auto vecAddr =
            regs<RegsMc68000>()->vecAddr(InstMc68000::VECNUM_ILLEGAL);
    auto s = resumeCycle();
    while (true) {
        // First: a slot that timed out holds the status and address of
        // its use a ring ago.
        if (s->halt()) {
            // STOP: no bus cycle until an interrupt.
            if (_devs->loop() && haltSwitch())
                return suspend(nullptr);
            s = prepareCycleOf<BOARD>(wait_poll_clocks);
            continue;
        }
        // The address first: it rules out nearly every cycle.
        if ((s->addr & ~1) == vecAddr && memRead(s) && s->dataSpace()) {
            RegsMc68000::Frame frame;
            const Signals *from = nullptr;
            if (findFrame(s, frame, from)) {
                // ILLEGAL stacks its own address.
                regs<RegsMc68000>()->parkInVector(vecAddr, frame);
                Cycles::discard(from);
                return true;
            }
        }
        completeCycleOf<BOARD>(s);
        const auto serviced = _devs->loop();
        s = prepareCycleOf<BOARD>(halt_clocks);
        // Only now, with a cycle recorded: suspend() completes it first.
        // Not at an acknowledge: the program's own interrupt is under way.
        if (!serviced || s->iack())
            continue;
        if (haltSwitch())
            return suspend(s->halt() ? nullptr : s);
    }
}

}  // namespace mc68000
}  // namespace debugger
#endif /* __PINS_MC68000_CYCLE_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
