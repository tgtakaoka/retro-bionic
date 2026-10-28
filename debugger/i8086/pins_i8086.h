#ifndef __PINS_I8086_H__
#define __PINS_I8086_H__

// The P8086 and D8087 boards (schematics/p8086), in maximum mode. The 8088
// and V20 carry A8-A15 on AD8-AD15's pins, and HIGH where the 8086 has
// #BHE.
#define PORT_AD 6    /* GPIO6 */
#define AD_gp 16     /* P6.16-P6.31 */
#define AD_gm 0xFFFF /* P6.16-P6.31 */
#define AD_vp 0      /* AD0-AD15 */
#define PORT_ADL 6   /* GPIO6 */
#define ADL_gp 16    /* P6.16-P6.23 */
#define ADL_gm 0xFF  /* P6.16-P6.23 */
#define ADL_vp 0     /* AD0-AD7 */
#define PIN_AD0 19   /* P6.16 P10 */
#define PIN_AD1 18   /* P6.17 P11 */
#define PIN_AD2 14   /* P6.18 P12 */
#define PIN_AD3 15   /* P6.19 P13 */
#define PIN_AD4 40   /* P6.20 P14 */
#define PIN_AD5 41   /* P6.21 P15 */
#define PIN_AD6 17   /* P6.22 P16 */
#define PIN_AD7 16   /* P6.23 P17 */
#define PIN_AD8 22   /* P6.24 P20 */
#define PIN_AD9 23   /* P6.25 P21 */
#define PIN_AD10 20  /* P6.26 P22 */
#define PIN_AD11 21  /* P6.27 P23 */
#define PIN_AD12 38  /* P6.28 P24 */
#define PIN_AD13 39  /* P6.29 P25 */
#define PIN_AD14 26  /* P6.30 P26 */
#define PIN_AD15 27  /* P6.31 P27 */
// A16-A19 carry S3-S6 from T2 on; QS0, #S1, #S0 and #TEST share their port.
#define PORT_AH 7   /* GPIO7 */
#define AH_gp 0     /* P7.00-P7.03 */
#define AH_gm 0xF   /* P7.00-P7.03 */
#define AH_vp 16    /* A16-A19 */
#define PORT_ST 7   /* GPIO7 */
#define ST_gp 0     /* P7.00-P7.03 */
#define ST_gm 0xF   /* P7.00-P7.03 */
#define ST_vp 0     /* S3-S6 */
#define PIN_A16S 10 /* P7.00 P30 */
#define PIN_A17S 12 /* P7.01 P31 */
#define PIN_A18S 11 /* P7.02 P32 */
#define PIN_A19S 13 /* P7.03 P33 */
#define PIN_QS0 8   /* P7.16 P34 */
#define PIN_S1 7    /* P7.17 P35 */
#define PIN_S0 36   /* P7.18 P36 */
#define PIN_TEST 37 /* P7.19 P37, the D8087 board's 8087 BUSY */
// #RD, #LOCK, QS1, #BHE and #S2 in one read of GPIO9; Signals keeps the
// strobes #S2-#S0 stand for in the same bits.
#define PORT_CNTL 9  /* GPIO9 */
#define CNTL_gp 4    /* P9.04-P9.08 */
#define CNTL_gm 0x1F /* P9.04-P9.08 */
#define CNTL_vp 0
#define PIN_RD 2     /* P9.04 P40 */
#define PIN_LOCK 3   /* P9.05 P41 */
#define PIN_QS1 4    /* P9.06 P42 */
#define PIN_BHE 33   /* P9.07 P43 */
#define PIN_S2 5     /* P9.08 P46 */
#define PIN_INT 0    /* P6.03 P44, the D8087 board's 8087 INT */
#define PIN_MNMX 1   /* P6.02 P45 */
#define PIN_CLK 29   /* P9.31 P47 */
#define PIN_NMI 6    /* P7.10 P50 */
#define PIN_INTR 9   /* P7.11 P51 */
#define PIN_READY 32 /* P7.12 P52 */
#define PIN_RESET 28 /* P8.18 P53 */
#define PIN_RQGT0 31 /* P8.22 P54, the CPU's and the 8087's */
#define PIN_RQGT1 30 /* P8.23 P55 */
// #S1 and #S0; #S2 is CNTL's.
#define PORT_S01 7 /* GPIO7 */
#define S01_gp 17  /* P7.17-P7.18 */
#define S01_gm 0x3 /* P7.17-P7.18 */
#define S01_vp 0   /* #S1, #S0 */

#include "pins.h"
#include "pwm_clock.h"
#include "signals_i8086.h"

namespace debugger {
namespace i8086 {

// CLK free-runs from FlexPWM3, as the NMOS 8087 and the V20/V30 need, and
// the debugger holds each bus cycle in wait states with READY low.
struct PinsI8086 final : Pins {
    // |fpu|: the D8087 board's 8087 shares the bus. Reset tells the 8088
    // and V20 from the 8086 and V30, and the NEC parts from Intel's.
    PinsI8086(bool fpu);
    bool bus8() const { return _bus8; }
    bool vSeries() const { return _v_series; }

    // The clock runs on; READY holds the parked CPU.
    void idle() override;
#ifdef TUNE_CLOCK
    uint32_t clockInput() const override { return _clockInput; }
    bool setClockInput(uint32_t hz) override;
#endif
    bool step(bool show) override;
    void run() override;

    void printCycles() override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;
    void setBreakInst(uint32_t addr) const override;

    // Bytes the CPU reads from, or writes to, the debugger instead of
    // memory while an injected sequence runs.
    struct Overlay {
        void clear() { _num = 0; }
        void put(uint32_t addr, uint8_t data);
        // Whether |addr| has a byte; it goes to |data|.
        bool get(uint32_t addr, uint8_t &data) const;
        uint16_t wordAt(uint32_t addr) const;
        // Whether the CPU read every byte put since clear().
        bool allRead() const;
        void markRead(uint32_t addr);

    private:
        static constexpr uint_fast8_t MAX = 40;
        uint32_t _addr[MAX];
        uint8_t _data[MAX];
        bool _read[MAX];
        uint_fast8_t _num = 0;
        int find(uint32_t addr) const;
    };
    Overlay &overlay() { return _overlay; }

    // Runs |inst| on the CPU parked at |org|, its reads answered from
    // |inst| by address, then from the overlay, then from memory; every
    // write goes to the overlay. Parks at the read of |exit| once |inst|
    // was read through, or, with EXIT_OVERLAY, once the CPU read every
    // byte the overlay held before the run. |org| becomes the park.
    static constexpr uint32_t EXIT_OVERLAY = UINT32_MAX;
    void execInst(const uint8_t *inst, uint_fast8_t len, uint32_t &org,
            uint32_t exit);

private:
    const bool _fpu;
    PwmClock<PIN_CLK> _clock;
#ifdef TUNE_CLOCK
    // CLK's rate, which K sets.
    uint32_t _clockInput;
#endif
    bool _bus8 = false;
    bool _v_series = false;
    // The next cycle's T1, read ahead by completeCycle().
    Signals _latch;
    uint8_t _latchStatus;
    bool _latched = false;

    void resetPins() override;
    Signals *prepareCycle();
    bool latchCycle();
    void applyChipClock();
    Signals *resumeCycle(uint32_t addr);
    Signals *completeCycle(Signals *s);
    void retrieveData(Signals *s);
    void storeData(Signals *s);
    // Parks at the reset fetch, |bus8| as the CPU showed it there; false
    // if the CPU's first cycle is not that fetch.
    bool parkAtReset(Signals *s, bool bus8);
    Overlay _overlay;
    // The cycle a restore left the CPU in, for step() and run() to go on
    // with.
    Signals *_next = nullptr;
    // A trap's vector, and the frame it pushed, lowest byte first.
    uint8_t _vector = 0;
    uint32_t _frameAt[6];
    uint8_t _frame[6];
    uint_fast8_t _frameBytes = 0;

    const SignalsImpl *findBacktraceStart() override;
    void printBacktrace() override;

    // A read's bytes from the window, the overlay or memory.
    void injectRead(
            Signals *s, const uint8_t *inst, uint_fast8_t len, uint32_t org);
    void recordFrame(const Signals *s);
    enum Mode : uint8_t {
        STEP,
        RUN,
    };
    bool untilTrap(Signals *s, Mode mode);
    bool parkInTrap(Signals *s);
    friend struct RegsI8086;
    void restoreNext(Signals *s) { _next = s; }
};

}  // namespace i8086
}  // namespace debugger
#endif /* __PINS_I8086_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
