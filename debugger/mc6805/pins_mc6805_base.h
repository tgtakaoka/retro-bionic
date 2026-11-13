#ifndef __PINS_MC6805_BASE_H__
#define __PINS_MC6805_BASE_H__

#include "debugger.h"
#include "mems_mc6805.h"
#include "pins_mc6805.h"
#include "regs_mc6805.h"

namespace debugger {
namespace mc6805 {

// The bus loops, calling PINS_T's cycle functions directly.
template <typename PINS_T>
struct PinsMc6805Base : PinsMc6805 {
    void resetPins() override;
    bool step(bool show) override;
    void run() override;

    void injectReads(const uint8_t *inst, uint_fast8_t len, uint_fast8_t cycles,
            bool discard = false) override;
    uint16_t captureWrites(
            uint8_t *buf, uint_fast8_t len, bool discard = false) override;
    Signals *inject(uint8_t data);


protected:

    Signals *cycle();
    Signals *suspend(Signals *s);
    Signals *wakeUp(Signals *s);
    // Realign to the bus after STOP, starting and ending between cycles,
    // giving up after |clocks|.
    void resyncBus(uint16_t) {}
    bool rawStep();
    void loop();

private:
    // Longest instruction, with margin; no fetch by then means WAIT or STOP.
    static constexpr auto FETCH_CYCLES = 64;
    // Covers STOP's oscillator start-up delay before the interrupt push.
    static constexpr auto WAKE_CYCLES = 4096;

    PINS_T *pins() { return static_cast<PINS_T *>(this); }
};

template <typename PINS_T>
void PinsMc6805Base<PINS_T>::resetPins() {
    pins()->resetCpu();
    // resetCpu() leaves the CPU fetching at the external dummy vector,
    // where save() can inject its SWI.
    _regs->save();
    _regs->setIp(mems<MemsMc6805>()->resetVector());
}

template <typename PINS_T>
Signals *PinsMc6805Base<PINS_T>::cycle() {
    return pins()->completeCycle(pins()->prepareCycle());
}

template <typename PINS_T>
Signals *PinsMc6805Base<PINS_T>::inject(uint8_t data) {
    return pins()->completeCycle(pins()->prepareCycle()->inject(data));
}

template <typename PINS_T>
void PinsMc6805Base<PINS_T>::injectReads(const uint8_t *inst, uint_fast8_t len,
        uint_fast8_t cycles, bool discard) {
    // inject |inst|, then run |cycles| more bus cycles
    auto s = pins()->currCycle();
    for (uint_fast8_t inj = 0; inj < len; ++inj) {
        pins()->completeCycle(s->inject(inst[inj]));
        if (discard)
            Cycles::discard(s);
        s = pins()->prepareCycle();
    }
    for (uint_fast8_t inj = 0; inj < cycles; ++inj) {
        pins()->completeCycle(s);
        if (discard)
            Cycles::discard(s);
        s = pins()->prepareCycle();
    }
}

template <typename PINS_T>
uint16_t PinsMc6805Base<PINS_T>::captureWrites(
        uint8_t *buf, uint_fast8_t len, bool discard) {
    // capture |len| writes; an SWI pushes its five within its ten cycles,
    // so give up past that rather than wedge the board
    constexpr auto capture_cycles = 32;
    uint16_t addr = 0;
    auto s = pins()->currCycle();
    for (uint_fast8_t cap = 0, n = 0; cap < len; ++n) {
        if (n >= capture_cycles) {
            cli.println("?halt: no writes");
            break;
        }
        pins()->completeCycle(s->capture());
        if (s->write()) {
            if (cap == 0)
                addr = s->addr;
            if (cap < len)
                buf[cap++] = s->data;
        }
        if (discard)
            Cycles::discard(s);
        s = pins()->prepareCycle();
    }
    return addr;
}

#ifdef PROFILE_CYCLES
// For scripts/record-cycles.py: stop at any SWI, keep every cycle of it,
// and give up well before the ring wraps; the debug pin frames the run, to
// trigger a capture on.
template <typename PINS_T>
void PinsMc6805Base<PINS_T>::loop() {
    constexpr auto MAX_CYCLES = 96;
    const auto vec_swi = mems<MemsMc6805>()->vecSwi();
    const auto r = regs<RegsMc6805>();
    assert_debug();
    for (auto n = 0;; ++n) {
        _devs->loop();
        auto s = pins()->rawPrepareCycle();
        if (s->addr == vec_swi && r->captureContext(s->prev(5))) {
            negate_debug();
            r->captureExtra(r->nextIp() - 1);  // offset SWI
            restoreBreakInsts();
            disassembleCycles();
            return;
        }
        if (n >= MAX_CYCLES || haltSwitch()) {
            negate_debug();
            // Before suspend() adds its cycles, which can wrap the ring.
            disassembleCycles();
            cli.println(n >= MAX_CYCLES ? "?cycles" : "?halt");
            restoreBreakInsts();
            s = suspend(s);
            if (pins()->is_internal(s->addr)) {
                // Can't inject instruction for context save
                pins()->resetCpu();
                _regs->setIp(s->addr);
            } else if (!_woken) {
                _regs->save();
            }
            return;
        }
        pins()->completeCycle(s);
        if (s->fetch())
            _lastOpcode = s->data;
    }
}
#else
template <typename PINS_T>
void PinsMc6805Base<PINS_T>::loop() {
    const auto vec_swi = mems<MemsMc6805>()->vecSwi();
    while (true) {
        const auto serviced = _devs->loop();
        auto s = pins()->prepareCycle();
        if (s->addr == vec_swi && checkBreakPoint(s))
            return;
        if (serviced && haltSwitch()) {
            restoreBreakInsts();
            s = suspend(s);
            if (pins()->is_internal(s->addr)) {
                // Can't inject instruction for context save
                pins()->resetCpu();
                _regs->setIp(s->addr);
            } else {
                disassembleCycles();
                if (!_woken)
                    _regs->save();
            }
            return;
        }
        pins()->completeCycle(s);
        if (s->fetch())
            _lastOpcode = s->data;
    }
}
#endif

template <typename PINS_T>
Signals *PinsMc6805Base<PINS_T>::suspend(Signals *s) {
    _woken = false;
    if (s == nullptr)
        s = pins()->currCycle();
    for (auto n = 0; !s->fetch(); ++n) {
        if (n >= FETCH_CYCLES)
            return wakeUp(s);
        pins()->completeCycle(s);
        s = pins()->prepareCycle();
    }
    return s;
}

// WAIT and STOP fetch nothing until an interrupt, and both clear the I
// mask: take an IRQ, whose push stands in for save()'s SWI.
template <typename PINS_T>
Signals *PinsMc6805Base<PINS_T>::wakeUp(Signals *s) {
    pins()->completeCycle(s);
    assertInt(0);
    // After STOP the bus clock restarts at another phase against the
    // debugger's; after WAIT it doesn't.
    if (_inst->isStop(_lastOpcode))
        pins()->resyncBus(5 * WAKE_CYCLES);  // 5 OSC1 clocks per bus cycle
    uint_fast8_t writes = 0;
    for (auto n = 0; writes < 5; ++n) {
        s = pins()->prepareCycle();
        if (n >= WAKE_CYCLES) {
            negateInt(0);
            cli.println("?halt: CPU didn't wake");
            return s;
        }
        pins()->completeCycle(s->capture());
        writes = s->write() ? writes + 1 : 0;
    }
    s = pins()->prepareCycle();
    negateInt(0);
    regs<RegsMc6805>()->captureContext(s->prev(5));
    static constexpr uint8_t VECTOR[] = {0x10, 0x00};
    injectReads(VECTOR, sizeof(VECTOR), 1);  // V:v:n, parked at 1000
    _woken = true;
    return pins()->currCycle();
}

template <typename PINS_T>
void PinsMc6805Base<PINS_T>::run() {
    _regs->restore();
    Cycles::reset();
    saveBreakInsts();
    // CPU is stopped at fetch
    pins()->completeCycle(pins()->currCycle());
    startRunTimer();
    pins()->loop();
    stopRunTimer();
    // Every way out of loop(), not only a break point, must take the
    // SWIs back out of memory.
    restoreBreakInsts();
}

template <typename PINS_T>
bool PinsMc6805Base<PINS_T>::rawStep() {
    const auto pc = _regs->nextIp();
    if (pins()->is_internal(pc))
        return false;
    auto s = pins()->currCycle(pc);
    const auto inst = _mems->get_prog(s->addr);
    if (!_inst->valid(inst))
        return false;
    _lastOpcode = inst;
    pins()->completeCycle(s);
    suspend(pins()->prepareCycle());
    return true;
}

template <typename PINS_T>
bool PinsMc6805Base<PINS_T>::step(bool show) {
    Cycles::reset();
    _regs->restore();
    if (show)
        Cycles::reset();
    if (pins()->rawStep()) {
        if (show)
            printCycles();
        if (!_woken)
            _regs->save();
        return true;
    }
    return false;
}

}  // namespace mc6805
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
