#ifndef __PINS_MC68HC12_H__
#define __PINS_MC68HC12_H__

#define PORT_AD 6           /* GPIO6 */
#define AD_gp 16            /* P6.16-P6.31 */
#define AD_gm 0xFFFF        /* P6.16-P6.31 */
#define AD_vp 0             /* AD0-AD15 */
#define PIN_AD0 19          /* P6.16 */
#define PIN_AD1 18          /* P6.17 */
#define PIN_AD2 14          /* P6.18 */
#define PIN_AD3 15          /* P6.19 */
#define PIN_AD4 40          /* P6.20 */
#define PIN_AD5 41          /* P6.21 */
#define PIN_AD6 17          /* P6.22 */
#define PIN_AD7 16          /* P6.23 */
#define PIN_AD8 22          /* P6.24 */
#define PIN_AD9 23          /* P6.25 */
#define PIN_AD10 20         /* P6.26 */
#define PIN_AD11 21         /* P6.27 */
#define PIN_AD12 38         /* P6.28 */
#define PIN_AD13 39         /* P6.29 */
#define PIN_AD14 26         /* P6.30 */
#define PIN_AD15 27         /* P6.31 */
#define PORT_CNTL 9         /* GPIO9 */
#define CNTL_gp 4           /* P9.04-P9.07 */
#define CNTL_gm 0xF         /* P9.04-P9.07 */
#define CNTL_vp 0           /* CNTL0-CNTL3 */
#define PIN_DBE 2           /* P9.04 */
#define PIN_RW 3            /* P9.05 */
#define PIN_MODA 4          /* P9.06 */
#define PIN_IPIPE0 PIN_MODA /* P9.06 */
#define PIN_MODB 33         /* P9.07 */
#define PIN_IPIPE1 PIN_MODB /* P9.07 */
#define CNTL_DBE 0x1        /* CNTL0 */
#define CNTL_RW 0x2         /* CNTL1 */
#define CNTL_IPIPE_gp 2     /* CNTL2-CNTL3 */
#define PIN_BKGD 0          /* P6.03 */
#define PIN_DIVBYP 1        /* P6.02 */
#ifndef PIN_EXTAL
#define PIN_EXTAL 5         /* P9.08 */
#endif
#ifndef PIN_E
#define PIN_E 29            /* P9.31 */
#endif
#define PIN_IRQ 6           /* P7.10 */
#define PIN_XIRQ 9          /* P7.11 */
#define PIN_PT7 32          /* P7.12 */
#define PIN_RESET 28        /* P8.18 */
#ifndef PIN_LSTRB
#define PIN_LSTRB 31        /* P8.22 */
#endif
#define PIN_PT0 30          /* P8.23 */
#define PIN_RXD 10          /* P7.00 */
#define PIN_TXD 12          /* P7.01 */
#define PIN_PS2 11          /* P7.02 */
#define PIN_PS3 13          /* P7.03 */
#define PIN_PS4 8           /* P7.16 */
#define PIN_PS5 7           /* P7.17 */
#define PIN_PS6 36          /* P7.18 */
#define PIN_PS7 37          /* P7.19 */

#include "pins.h"
#include "signals_mc68hc12.h"

namespace debugger {
namespace mc68hc12 {

struct Mc68hc12Init;
struct RegsMc68hc12;

struct PinsMc68hc12 : Pins {
    PinsMc68hc12(Mc68hc12Init &init) : PinsMc68hc12(init, 2, RATED_HZ) {
#ifdef TUNE_CLOCK
        setClockInput(DEFAULT_HZ);
#endif
    }
#ifdef TUNE_CLOCK
    uint32_t clockInput() const override;
    bool setClockInput(uint32_t hz) override;
#endif

    // The bus cycle's delays in ns, shared by the family.
    struct Timing {
        uint16_t addr;   // quiet edge to address, R/W, #LSTRB, NOACC
        uint16_t extal;  // E rise to the quiet edge
        uint16_t data;   // quiet edge to #DBE, write or internal read data
        uint16_t setup;  // read data to the E fall
    };

    // The CPU12 is fully static: the clock simply stops.
    void idle() override {}
    bool step(bool show) override;
    void run() override;
    void printCycles() override { printCycles(nullptr); }
    void assertInt(uint8_t name = 0) override;
    void negateInt(uint8_t name = 0) override;
    void setBreakInst(uint32_t addr) const override;

    // Where the CPU is parked: it waits in the read of |park()|, the
    // word holding a "BRA *".
    uint16_t park() const { return _park; }
    // Where an injected sequence leaves off when not told: at the park.
    static constexpr uint32_t EXIT_PARK = UINT32_MAX;
    // A window of bytes the CPU reads by address: a vector or a stack
    // frame.
    struct Window {
        uint16_t at;
        const uint8_t *bytes;
        uint8_t len;
    };
    // What an injected sequence wrote, by address.
    struct Capture {
        static constexpr uint8_t MAX = 16;
        uint16_t addr[MAX];
        uint8_t data[MAX];
        uint8_t n;
        // The lowest address written, and the bytes from it in |buf|.
        uint16_t frame(uint8_t *buf, uint8_t len) const;
    };
    // Run |inst| from the park: a read of [park, park+len) is answered from
    // |inst|, one of |win| from it, anything else with NOPs, and the writes
    // are captured into |cap|, up to |max| bytes, and kept from memory. It
    // ends at the fetch of |exit| once |inst| and |win| were read and |max|
    // bytes captured, and the CPU waits in that read.
    void execInst(const uint8_t *inst, uint8_t len, Capture *cap = nullptr,
            uint8_t max = 0, uint32_t exit = EXIT_PARK,
            const Window *win = nullptr);

protected:
    // |stepAt|: the cycle after restore() at whose E fall a step asserts
    // #XIRQ. |ratedHz|: the rated bus clock K's rate stands for.
    PinsMc68hc12(Mc68hc12Init &init, uint8_t stepAt, uint32_t ratedHz);

    // The MC68HC912BD32's rated bus clock.
    static constexpr uint32_t RATED_HZ = 8000000;
    // K's rate out of reset: the family's delays halved, as fast as the
    // bench went.
    static constexpr uint32_t DEFAULT_HZ = 16000000;

    // Where the CPU waits between injected sequences, and where it stores
    // what it reads for the debugger: both outside, away from any program.
    static constexpr uint16_t PARK = 0xFF80;
    // The lowest vector, and how far past a sequence the queue fetches.
    static constexpr uint16_t VECTORS = 0xFF10;
    static constexpr uint16_t PREFETCH = 6;

    Mc68hc12Init &_init;
    const uint8_t _stepAt;
    const uint32_t _ratedHz;
    // The CPU waits in the read |_heldSignals|, at |_park|.
    bool _held = false;
    // Until MISC clears the stretch, an external access holds E high for
    // three more cycles.
    bool _stretch = true;
    bool _xirq = false;
    // prepareCycle() saw the cycle end: a stretched free or internal read.
    bool _done = false;
    // Cycles in a row without E while stretched.
    uint16_t _noE = 0;
    // rawStep() waits for RTI's check to assert #XIRQ.
    bool _stepArm = false;
    uint8_t _stepCycles = 0;
    // findBacktraceStart() marked the ring before it was cut.
    bool _replayed = false;
    uint16_t _park;
    Signals _heldSignals;
    // The writes since the last external read, and their bytes: WAI's
    // stacking before it waits.
    uint8_t _writes = 0;
    Capture _stacked;

#ifdef TUNE_CLOCK
    // The delays K scales: per mille of the rated bus clock.
    uint16_t _clockPermille = 1000;
    Timing _timing;
    void setTiming();
#endif
    template <uint16_t Timing::*F>
    void delay() const;

    void resetPins() override;
    // After reset: the reset vector points at the park, the system is set
    // up and the registers saved.
    void startPark();
    // After configSystem(), the last cycles with the reset's stretch.
    virtual void systemConfigured() {}
    // The cycle engine: each board instantiates it, so a cycle calls the
    // hooks below directly; elsewhere it is reached through these virtuals.
    virtual Signals *prepareCycle();
    virtual Signals *completeCycle(Signals *s);
    virtual const Signals *loop();
    template <typename BOARD>
    Signals *prepareCycleOf();
    template <typename BOARD>
    Signals *completeCycleOf(Signals *s);
    template <typename BOARD>
    const Signals *loopOf();
    Signals *stopped(Signals *s);
    Signals *noBusCycle(Signals *s);

    // The engine's hooks; a board hides them with its own.
    // Cycles in a row without E, while stretched, before the CPU counts as
    // stopped: E runs on every cycle.
    static constexpr uint16_t STOP_CYCLES = 1;
    // Before the E rise: whether the cycle is free.
    bool sampleFree() const { return false; }
    // After the E rise: whether the debugger answers the cycle.
    void classify(Signals *s, bool) const { s->getDbe(); }
    // Before the E fall, for a cycle the debugger doesn't drive.
    void pickupData(Signals *s) const {
        if (s->write())
            s->getData();
    }
    // Marks the instruction starts and program words of |cycles| cycles.
    virtual void replay(Signals *begin, uint_fast8_t cycles);
    // Asserts a step's #XIRQ at the |_stepAt|th cycle after restore().
    void armStep() {
        if (_stepArm && ++_stepCycles == _stepAt) {
            assert_xirq();
            _stepArm = false;
        }
    }
    void hold(const Signals *s, uint16_t park);
    uint16_t readBus(const Signals *s) const;
    void writeBus(const Signals *s) const;
    void execute(uint16_t org, const uint8_t *inst, uint8_t len, Capture *cap,
            uint8_t max, uint32_t exit, const Window *win, bool vector);
    void exception(Signals *s, bool breakTrap, Capture *stacked = nullptr);
    void track(const Signals *s);
    bool rawStep();

    void assert_xirq();
    void negate_xirq();

    void printCycles(const Signals *end);
    const SignalsImpl *findBacktraceStart() override;
    void printBacktrace() override;
};

}  // namespace mc68hc12
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
