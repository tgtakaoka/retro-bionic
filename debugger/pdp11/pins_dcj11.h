#ifndef __PINS_DCJ11_H__
#define __PINS_DCJ11_H__

// The DCJ11 board (schematics/dcj11).
#define PORT_DAL 6    /* GPIO6 */
#define DAL_gp 16     /* P6.16-P6.31 */
#define DAL_gm 0xFFFF /* P6.16-P6.31 */
#define DAL_vp 0      /* DAL0-DAL15 */
#define PIN_DAL0 19   /* P6.16 */
#define PIN_DAL1 18   /* P6.17 */
#define PIN_DAL2 14   /* P6.18 */
#define PIN_DAL3 15   /* P6.19 */
#define PIN_DAL4 40   /* P6.20 */
#define PIN_DAL5 41   /* P6.21 */
#define PIN_DAL6 17   /* P6.22 */
#define PIN_DAL7 16   /* P6.23 */
#define PIN_DAL8 22   /* P6.24 */
#define PIN_DAL9 23   /* P6.25 */
#define PIN_DAL10 20  /* P6.26 */
#define PIN_DAL11 21  /* P6.27 */
#define PIN_DAL12 38  /* P6.28 */
#define PIN_DAL13 39  /* P6.29 */
#define PIN_DAL14 26  /* P6.30 */
#define PIN_DAL15 27  /* P6.31 */
// Two 74HCS153 put DAL16-DAL19 (select 0), DAL20, DAL21, BS0, BS1
// (select 1) or AIO0-AIO3 (select 2) on P30-P33.
#define PORT_MUX 7    /* GPIO7 */
#define MUX_gp 0      /* P7.00-P7.03 */
#define MUX_gm 0xF    /* P7.00-P7.03 */
#define MUX_vp 0
#define PIN_MUX0 10   /* P7.00 P30 */
#define PIN_MUX1 12   /* P7.01 P31 */
#define PIN_MUX2 11   /* P7.02 P32 */
#define PIN_MUX3 13   /* P7.03 P33 */
#define PIN_ALE 8     /* P7.16 P34 */
#define PIN_SCTL 7    /* P7.17 P35 */
#define PIN_BUFCTL 36 /* P7.18 P36 */
#define PIN_CONT 37   /* P7.19 P37 */
#define PIN_MAP 2     /* P9.04 P40 */
#define PIN_PRDC 3    /* P9.05 P41 */
#define PIN_STRB 33   /* P9.07 P43 */
// The 74HCT259's data, strobe and selects; ASEL0 and ASEL1 also select
// the 153s.
#define PIN_LDATA 1   /* P6.02 P45 */
#define PIN_ABORT 5   /* P9.08 P46 */
#define PIN_CLK 29    /* P9.31 P47 */
#define PIN_DV 6      /* P7.10 P50 */
#define PIN_SEL2 9    /* P7.11 P51 */
#define PIN_LSTRB 32  /* P7.12 P52 */
#define PIN_INIT 28   /* P8.18 P53 */
#define PIN_ASEL0 31  /* P8.22 P54 */
#define PIN_ASEL1 30  /* P8.23 P55 */

#include "pins_pdp11.h"
#include "pwm_clock.h"
#include "signals_dcj11.h"

namespace debugger {
namespace dcj11 {

struct PinsDcj11 final : pdp11::PinsPdp11 {
    PinsDcj11();

    void idle() override {}
#ifdef TUNE_CLOCK
    uint32_t clockInput() const override;
    bool setClockInput(uint32_t hz) override;
#endif
    bool step(bool show) override;
    void run() override;

    void printCycles() override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;
    void setBreakInst(uint32_t addr) const override;

    void execInst(const uint16_t *inst, uint_fast8_t words, uint16_t &org,
            uint32_t exit = EXIT_END, const uint16_t *stack = nullptr,
            uint16_t stackAt = 0, uint_fast8_t stackWords = 0) override;
    void captureWrites(const uint16_t *inst, uint_fast8_t words,
            uint16_t *buf, uint_fast8_t max, uint16_t &org) override;

private:
    // A run's clock; reset, steps and injected sequences are bit-banged.
    using Clock = PwmClock<PIN_CLK>;
    Clock _clock;
    uint32_t _clockHz;
    // The cycle the CPU is parked in: past #ALE, DAL no longer has it.
    Signals _parked;
    // A device read whose side effects wait for the next transaction:
    // a read-modify-write's read must not take a received character.
    uint32_t _devRead = 0;
    bool _devPending = false;
    // The trap vector the debugger answers, 0 for none.
    uint16_t _trapVector = 0;

    void resetPins() override;
    const SignalsImpl *findBacktraceStart() override;
    void printBacktrace() override;
    bool discarded(uint32_t i) const;
    uint32_t walkInsns(uint32_t nth) const;

    Signals *prepareCycle();
    Signals *resumeCycle();
    Signals *completeCycle(Signals *s);
    void retrieveData(Signals *s);
    void storeData(Signals *s);
    void settleDevRead(const Signals *s);
    void execute(const uint16_t *inst, uint_fast8_t words, uint16_t *buf,
            uint_fast8_t max, uint16_t &org, uint32_t exit,
            const uint16_t *stack, uint16_t stackAt, uint_fast8_t stackWords);
    bool parkAfterFrame(Signals *s, bool step);
    bool suspend(Signals *s);
    bool rawStep();
    bool loop();
    Signals *catchCycle(uint32_t cycles);
    Signals *waitCycle();
    Signals *stopWatching();
    void serve(Signals *s);
    void release(Signals *s);
    void startClock();
    void stopClock();
};

}  // namespace dcj11
}  // namespace debugger
#endif /* __PINS_DCJ11_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
