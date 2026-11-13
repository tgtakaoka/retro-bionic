#ifndef __PINS_MC6809_BASE_H__
#define __PINS_MC6809_BASE_H__

#define PORT_D 6     /* GPIO6 */
#define D_gp 16      /* P6.16-P6.23 */
#define D_gm 0xFF    /* P6.16-P6.23 */
#define D_vp 0       /* D0-D7 */
#define PIN_D0 19    /* P6.16 */
#define PIN_D1 18    /* P6.17 */
#define PIN_D2 14    /* P6.18 */
#define PIN_D3 15    /* P6.19 */
#define PIN_D4 40    /* P6.20 */
#define PIN_D5 41    /* P6.21 */
#define PIN_D6 17    /* P6.22 */
#define PIN_D7 16    /* P6.23 */
#define PORT_AL 6    /* GPIO6 */
#define AL_gp 24     /* P6.24-P6.31 */
#define AL_gm 0xFF   /* P6.24-P6.31 */
#define AL_vp 0      /* A0-A7 */
#define PIN_AL0 22   /* P6.24 */
#define PIN_AL1 23   /* P6.25 */
#define PIN_AL2 20   /* P6.26 */
#define PIN_AL3 21   /* P6.27 */
#define PIN_AL4 38   /* P6.28 */
#define PIN_AL5 39   /* P6.29 */
#define PIN_AL6 26   /* P6.30 */
#define PIN_AL7 27   /* P6.31 */
#define PORT_AM 7    /* GPIO7 */
#define AM_gp 0      /* P7.00-P7.03 */
#define AM_gm 0xF    /* P7.00-P7.03 */
#define AM_vp 8      /* A8-A11 */
#define PIN_AM8 10   /* P7.00 */
#define PIN_AM9 12   /* P7.01 */
#define PIN_AM10 11  /* P7.02 */
#define PIN_AM11 13  /* P7.03 */
#define PORT_AH 7    /* GPIO7 */
#define AH_gp 16     /* P7.16-P7.19 */
#define AH_gm 0xF    /* P7.16-P7.19 */
#define AH_vp 12     /* A12-A15 */
#define PIN_AH12 8   /* P7.16 */
#define PIN_AH13 7   /* P7.17 */
#define PIN_AH14 36  /* P7.18 */
#define PIN_AH15 37  /* P7.19 */
#define PORT_CNTL 9  /* GPIO9 */
#define CNTL_gp 4    /* P9.04-P9.07 */
#define CNTL_gm 0xF  /* P9.04-P9.07 */
#define CNTL_vp 0    /* CNTL0-CNTL3 */
#define PIN_RW 3     /* P9.05 */
#define PIN_BS 4     /* P9.06 */
#define PIN_BA 33    /* P9.07 */
#define CNTL_VMA 0x1 /* CNTL0 */
#define CNTL_RW 0x2  /* CNTL1 */
#define CNTL_BS 0x4  /* CNTL2 */
#define CNTL_BA 0x8  /* CNTL3 */
#define PIN_FIRQ 1   /* P6.02 */
#define PIN_IRQ 6    /* P7.10 */
#define PIN_NMI 9    /* P7.11 */
#define PIN_RESET 28 /* P8.18 */
#define PIN_HALT 30  /* P8.23 */

#include "debugger.h"
#include "devs_mc6809.h"
#include "digital_bus.h"
#include "inst_mc6809.h"
#include "mems.h"
#include "pins.h"
#include "regs_mc6809.h"
#include "signals_mc6809.h"

namespace debugger {
namespace mc6809 {

enum IntrName : uint8_t {
    INTR_IRQ = 0xF8,   // lo(VEC_IRQ)
    INTR_FIRQ = 0xF6,  // lo(VEC_FIRQ)
};

struct PinsMc6809Base : Pins {
    ~PinsMc6809Base() { delete _inst; }

    void idle() override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;
    void setBreakInst(uint32_t addr) const override;
    void printCycles() override { printCycles(nullptr); }

    void injectReads(const uint8_t *inst, uint8_t len, uint8_t cycles = 0);
    void captureWrites(uint8_t *buf, uint8_t len);
    uint8_t captureContext(uint8_t *context, uint16_t &sp);

protected:
    InstMc6809 *_inst;

    virtual Signals *rawCycle() const = 0;
    virtual Signals *cycle() const = 0;
    Signals *injectCycle(uint8_t data);
#ifdef PROFILE_CYCLES
    const Signals *_profileEnd;
#endif
    const Signals *stackFrame(const Signals *push) const;

    void printCycles(const Signals *end);
    virtual const Signals *findFetch(Signals *begin, const Signals *end);
    const SignalsImpl *findBacktraceStart() override;
    void printBacktrace() override;
};

// The bus loops, calling the part's final rawCycle() and cycle()
// directly; each part's .cpp instantiates it.
template <typename IMPL>
struct PinsMc6809Engine : PinsMc6809Base {
    bool step(bool show) override;
    void run() override;

protected:
    void resetPins() override;

private:
    void suspend(bool show);
    void loop();
    const IMPL *impl() const { return static_cast<const IMPL *>(this); }
};

void Signals::getHighAddr() {
    addr = busRead(AM) | busRead(AH);
}

void Signals::getLowAddr() {
    addr |= busRead(AL);
}

void Signals::getDirection() {
    // The MC6809 has no VMA; CNTL0 is its XTAL pin, or AVMA on the
    // MC6809E, which tells of the next cycle. Dummy cycles read FFFF,
    // which the cycle sequences match as x.
    cntl() = busRead(CNTL) | CNTL_VMA;
}

void Signals::getControl() {
    clearFetch();
}

bool Signals::vector() const {
    return (cntl() & (CNTL_BA | CNTL_BS)) == CNTL_BS;
}

void Signals::getData() {
    data = busRead(D);
}

void Signals::outData() const {
    busWrite(D, data);
    busMode(D, OUTPUT);
}

void Signals::inputMode() {
    busMode(D, INPUT);
}

// Longest instruction (HD6309 DIVQ) plus a context push, with margin.
constexpr auto SUSPEND_CYCLES = 100;

inline void assert_nmi() {
    digitalWriteFast(PIN_NMI, LOW);
}

inline void negate_nmi() {
    digitalWriteFast(PIN_NMI, HIGH);
}

inline bool asserted(uint8_t pin) {
    return (*portOutputRegister(pin) & digitalPinToBitMask(pin)) == 0;
}

template <typename IMPL>
void PinsMc6809Engine<IMPL>::resetPins() {
    Cycles::reset();

    // Clock up to the reset vector read.
    while (!impl()->cycle()->vector())
        ;
    // lo(reset vector)
    impl()->cycle();
    impl()->cycle();  // non-VMA
    _regs->reset();
    // SoftwareType must be determined before context save.
    auto softType = regs<RegsMc6809>()->checkSoftwareType();
    _inst->setSoftwareType(softType);
    _regs->save();
    _regs->setIp(_mems->read16(InstMc6809::VEC_RESET));
}

#ifdef PROFILE_CYCLES
// For scripts/record-cycles.py: stop at any SWI, keep every cycle of it,
// and give up well before the ring wraps; the debug pin frames the run, to
// trigger a capture on.
template <typename IMPL>
void PinsMc6809Engine<IMPL>::loop() {
    constexpr auto MAX_CYCLES = 96;
    const auto vec_swi = _inst->vec_swi();
    _profileEnd = nullptr;
    assert_debug();
    for (auto n = 0;; ++n) {
        devs<DevsMc6809>()->loop();
        const auto s = impl()->rawCycle();
        if (s->vector() && s->addr == vec_swi) {
            negate_debug();
            impl()->cycle();  // read SWI low(vector);
            impl()->cycle();  // non-VMA
            const auto frame = stackFrame(s->prev(2));
            regs<RegsMc6809>()->capture(frame);
            // Match up to the SWI's opcode fetch, as the normal build does.
            _profileEnd = frame->prev(2);
            return;
        }
        if (n >= MAX_CYCLES || haltSwitch()) {
            negate_debug();
            cli.println(n >= MAX_CYCLES ? "?cycles" : "?halt");
            suspend(true);
            return;
        }
    }
}
#else
template <typename IMPL>
void PinsMc6809Engine<IMPL>::loop() {
    const auto vec_swi = _inst->vec_swi();
    while (true) {
        const auto serviced = devs<DevsMc6809>()->loop();
        const auto s = impl()->cycle();
        if (s->vector() && s->addr == vec_swi) {
            impl()->cycle();  // read SWI low(vector);
            impl()->cycle();  // non-VMA
            const auto frame = stackFrame(s->prev(2));
            const auto pc = uint16(frame->next()->data, frame->data);
            const auto swi_vector = _mems->read16(vec_swi);
            if (isBreakPoint(pc) || swi_vector == vec_swi) {
                regs<RegsMc6809>()->capture(frame);
                Cycles::discard(frame->prev(2));
                return;
            }
        } else if (serviced && haltSwitch()) {
            suspend(true);
            return;
        }
    }
}
#endif

template <typename IMPL>
void PinsMc6809Engine<IMPL>::suspend(bool show) {
    assert_nmi();
reentry:
    auto s = Signals::put();
    for (auto n = 0;; ++n) {
        s = impl()->cycle();
        if (s->vector())  // NMI hi(vector)
            break;
        if (n >= SUSPEND_CYCLES) {
            negate_nmi();
            cli.println("?halt: no vector fetched");
            return;
        }
    }
    negate_nmi();
    if (s->addr != InstMc6809::VEC_NMI) {
        // SWI or an interrupt got in first; NMI follows at its handler.
        impl()->cycle();  // lo(vector)
        impl()->cycle();  // non-VMA
        goto reentry;
    }
    const auto frame = stackFrame(s->prev(2));
    if (!frame->write()) {
        // CWAI stacks the context up front, so NMI fetches its vector
        // without pushing. RTI back out of CWAI, then take a fresh NMI
        // edge.
        impl()->cycle();  // NMI lo(vector)
        impl()->cycle();  // non-VMA
        injectCycle(InstMc6809::RTI);
        assert_nmi();
        goto reentry;
    }
    impl()->cycle();  // NMI lo(vector)
    impl()->cycle();  // non-VMA
    regs<RegsMc6809>()->capture(frame, true);
    if (show) {
        // Keep prefetch cycle for bus cycle pattern matching.
        const auto last =
                frame->prev(regs<RegsMc6809>()->contextLength() == 14 ? 3 : 2);
        Cycles::discard(last);
    }
}

template <typename IMPL>
void PinsMc6809Engine<IMPL>::run() {
    _regs->restore();
    Cycles::reset();
    saveBreakInsts();
    startRunTimer();
    loop();
    stopRunTimer();
    restoreBreakInsts();
    disassembleCycles();
}

template <typename IMPL>
bool PinsMc6809Engine<IMPL>::step(bool show) {
    Cycles::reset();
    _regs->restore();
    if (show)
        Cycles::reset();
    // With IRQ or FIRQ pending and unmasked, its entry begins right after
    // the restoring RTI; the step's NMI would win and stay in place.
    const auto r = regs<RegsMc6809>();
    if ((asserted(PIN_IRQ) && !r->irqMasked()) ||
            (asserted(PIN_FIRQ) && !r->firqMasked())) {
        for (auto n = 0; n < SUSPEND_CYCLES; ++n) {
            if (impl()->cycle()->vector()) {  // hi(vector)
                impl()->cycle();              // lo(vector)
                impl()->cycle();              // non-VMA
                break;
            }
        }
    }
    suspend(show);
    if (show) {
        // Discard prefetch cycle.
        const auto end = Signals::put()->prev(1);
        printCycles(end);
    }
    return true;
}

}  // namespace mc6809
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
