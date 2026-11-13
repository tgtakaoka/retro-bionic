#ifndef __PINS_MC6800_BASE_H__
#define __PINS_MC6800_BASE_H__

#include "debugger.h"
#include "inst_mc6800.h"
#include "pins.h"
#include "regs_mc6800.h"
#include "signals_mc6800.h"

#define PORT_CNTL 9  /* GPIO9 */
#define CNTL_gp 4    /* P9.04-P9.07 */
#define CNTL_gm 0xF  /* P9.04-P9.07 */
#define CNTL_vp 0    /* CNTL0-CNTL3 */
#define PIN_RW 3     /* P9.05 */
#define CNTL_VMA 0x1 /* CNTL0 */
#define CNTL_RW 0x2  /* CNTL1 */
#define PIN_IRQ 6    /* P7.10 */
#define PIN_NMI 9    /* P7.11 */
#define PIN_RESET 28 /* P8.18 */

namespace debugger {
namespace mc6800 {

struct PinsMc6800Base : Pins {
    ~PinsMc6800Base() { delete _inst; }

    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;
    void setBreakInst(uint32_t addr) const override;
    void printCycles() override { printCycles(nullptr); }

    // Injects |inst| into the next |len| cycles, then NOPs up to |cycles|.
    virtual void injectReads(
            const uint8_t *inst, uint8_t len, uint8_t cycles = 0) = 0;
    // Captures the next |len| writes, kept from memory; |addr| gets the
    // first one's address.
    virtual void captureWrites(
            uint8_t *buf, uint8_t len, uint16_t *addr = nullptr) = 0;
    // Whether a non-VMA cycle comes between a context push and its vector
    // read.
    virtual bool nonVmaAfterContextSave() const { return true; }

protected:
    ArchMc6800 *_inst;
    uint8_t _writes;  // writes in a row: a context push counts up
#ifdef PROFILE_CYCLES
    const Signals *_profileEnd;
#endif

    // Longest instruction plus a context save, with margin.
    static constexpr auto SUSPEND_CYCLES = 64;
    static bool irq_asserted() {
        return (*portOutputRegister(PIN_IRQ) & digitalPinToBitMask(PIN_IRQ)) ==
               0;
    }
    static void assert_nmi();
    static void negate_nmi();

    void printCycles(const Signals *end);
    const Signals *findFetch(Signals *begin, const Signals *end);
    const SignalsImpl *findBacktraceStart() override;
    virtual void printBacktrace() override;
};

// The paths that run bus cycles, calling PINS' own cycles directly.
template <typename PINS>
struct PinsMc6800Cycles : PinsMc6800Base {
    void idle() override;
    bool step(bool show) override;
    void run() override;
    void injectReads(const uint8_t *inst, uint8_t len, uint8_t cycles) override;
    void captureWrites(uint8_t *buf, uint8_t len, uint16_t *addr) override;

protected:
    // The cycle after _devs->loop(), whose time counts toward its first
    // clock phase.
    Signals *loopCycle() { return pins()->cycle(); }
    Signals *injectCycle(uint8_t data);
    void suspend(bool show);
    void loop();

private:
    PINS *pins() { return static_cast<PINS *>(this); }
};

template <typename PINS>
Signals *PinsMc6800Cycles<PINS>::injectCycle(uint8_t data) {
    Signals::put()->inject(data);
    return pins()->cycle();
}

template <typename PINS>
void PinsMc6800Cycles<PINS>::injectReads(
        const uint8_t *inst, uint8_t len, uint8_t cycles) {
    for (auto inj = 0; inj < len; ++inj) {
        injectCycle(inst[inj]);
    }
    for (auto inj = len; inj < cycles; ++inj) {
        injectCycle(ArchMc6800::NOP);
    }
}

template <typename PINS>
void PinsMc6800Cycles<PINS>::captureWrites(
        uint8_t *buf, uint8_t len, uint16_t *addr) {
    for (auto cap = 0; cap < len;) {
        auto s = Signals::put();
        s->capture();
        pins()->cycle();
        if (s->write()) {
            if (cap == 0 && addr)
                *addr = s->addr;
            buf[cap++] = s->data;
        }
    }
}

// A 4-cycle BRA $ keeps the PC while the bus stays clocked; its cycles
// are dropped from the ring.
template <typename PINS>
void PinsMc6800Cycles<PINS>::idle() {
    auto s = Signals::put();
    s->inject(ArchMc6800::BRA);
    pins()->rawCycle();
    injectCycle(ArchMc6800::BRA_HERE);
    injectCycle(ArchMc6800::NOP);
    injectCycle(ArchMc6800::NOP);
    Cycles::discard(s);
}

#ifdef PROFILE_CYCLES
// For scripts/record-cycles.py: stop at any SWI, keep every cycle of it,
// and give up well before the ring wraps; the debug pin frames the run, to
// trigger a capture on.
template <typename PINS>
void PinsMc6800Cycles<PINS>::loop() {
    constexpr auto MAX_CYCLES = 96;
    const auto contextLength = regs<RegsMc6800>()->contextLength();
    _profileEnd = nullptr;
    assert_debug();
    for (auto n = 0;; ++n) {
        _devs->loop();
        pins()->rawCycle();
        if (_writes == contextLength) {
            const auto frame = Signals::put()->prev(_writes);
            if (pins()->nonVmaAfterContextSave())
                pins()->cycle();                  // non VMA cycle
            const auto vec_hi = pins()->cycle();  // read interrupt high(vector)
            if (vec_hi->addr == _inst->vec_swi()) {
                negate_debug();
                pins()->cycle();  // read interrupt low(vector)
                regs<RegsMc6800>()->capture(frame, false);
                // Match up to the SWI's opcode fetch, as the normal build does.
                _profileEnd =
                        frame->prev(pins()->nonVmaAfterContextSave() ? 1 : 2);
                return;
            }
            _writes = 0;
        } else if (n >= MAX_CYCLES || haltSwitch()) {
            negate_debug();
            cli.println(n >= MAX_CYCLES ? "?cycles" : "?halt");
            suspend(true);
            return;
        }
    }
}
#else
template <typename PINS>
void PinsMc6800Cycles<PINS>::loop() {
    const auto contextLength = regs<RegsMc6800>()->contextLength();
    while (true) {
        const auto serviced = _devs->loop();
        pins()->loopCycle();
        // A context push: the vector read after it tells an SWI's.
        if (_writes == contextLength) {
            const auto frame = Signals::put()->prev(_writes);
            if (pins()->nonVmaAfterContextSave())
                pins()->cycle();                  // non VMA cycle
            const auto vec_hi = pins()->cycle();  // read interrupt high(vector)
            const auto vec_swi = _inst->vec_swi();
            if (vec_hi->addr == vec_swi) {
                pins()->cycle();  // read interrupt low(vector)
                const auto pc = regs<RegsMc6800>()->capture(frame, false);
                const auto swi_vector = _mems->read16(vec_swi);
                // An SWI vector pointing at itself is a program's halt to
                // system.
                if (isBreakPoint(pc) || swi_vector == vec_swi) {
                    const auto discard =
                            pins()->nonVmaAfterContextSave() ? 1 : 2;
                    Cycles::discard(frame->prev(discard));
                    return;
                }
            }
            // WAI idles on non-VMA cycles, which leave _writes as is.
            _writes = 0;
        } else if (serviced && haltSwitch()) {
            suspend(true);
            return;
        }
    }
}
#endif

template <typename PINS>
void PinsMc6800Cycles<PINS>::suspend(bool show) {
    const auto contextLength = regs<RegsMc6800>()->contextLength();
    const auto vec_swi = _inst->vec_swi();
    const auto vec_nmi = _inst->vec_nmi();
    assert_nmi();
reentry:
    _writes = 0;
    // Wait for the NMI's context push, a write per stacked byte.
    for (auto n = 0; _writes < contextLength; ++n) {
        const auto s = pins()->cycle();
        if (s->valid() && s->read() && s->addr == vec_nmi) {
            // In WAI the context is already stacked, so NMI fetches its
            // vector without pushing. RTI back out of WAI, then take a
            // fresh NMI edge.
            negate_nmi();
            pins()->cycle();  // NMI lo(vector)
            injectCycle(ArchMc6800::RTI);
            assert_nmi();
            goto reentry;
        }
        if (n >= SUSPEND_CYCLES) {
            negate_nmi();
            cli.println("?halt: no context saved");
            return;
        }
    }
    // Capture registers pushed onto stack.
    const auto frame = Signals::put()->prev(_writes);
    // A push by WAI is followed by idle cycles until the NMI is taken,
    // so find the vector read rather than counting cycles to it.
    const Signals *v;
    for (auto n = 0;; ++n) {
        v = pins()->cycle();
        if (v->valid() && v->read() &&
                (v->addr == vec_swi || v->addr == vec_nmi))
            break;
        if (n >= SUSPEND_CYCLES) {
            negate_nmi();
            cli.println("?halt: no vector fetched");
            return;
        }
    }
    negate_nmi();
    // An SWI pushed first: give NMI a fresh edge and wait for its push.
    if (v->addr == vec_swi) {
        assert_nmi();
        pins()->cycle();  // SWI lo(vector);
        goto reentry;
    }
    pins()->cycle();  // NMI lo(vector)
    regs<RegsMc6800>()->capture(frame);
    if (show) {
        const auto discard = pins()->nonVmaAfterContextSave() ? 1 : 2;
        Cycles::discard(frame->prev(discard));
    }
}

template <typename PINS>
void PinsMc6800Cycles<PINS>::run() {
    _regs->restore();
    Cycles::reset();
    saveBreakInsts();
    startRunTimer();
    loop();
    stopRunTimer();
    restoreBreakInsts();
    disassembleCycles();
}

template <typename PINS>
bool PinsMc6800Cycles<PINS>::step(bool show) {
    Cycles::reset();
    _regs->restore();
    if (show)
        Cycles::reset();
    // A pending IRQ the restored CC lets in starts its entry before any
    // instruction, and an NMI raised now would take the vector: let the
    // IRQ fetch its vector first.
    if (irq_asserted() && !regs<RegsMc6800>()->irqMasked()) {
        const auto vec_irq = _inst->vec_irq();
        for (auto n = 0; n < SUSPEND_CYCLES; ++n) {
            const auto s = pins()->cycle();
            if (s->valid() && s->read() && s->addr == vec_irq)
                break;
        }
    }
    suspend(show);
    if (show)
        printCycles(Signals::put()->prev());
    return true;
}

}  // namespace mc6800
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
