#ifndef __PINS_I8086_H__
#define __PINS_I8086_H__

// The P8086 and D8087 boards (schematics/p8086), in maximum mode. The 8088
// and V20 have HIGH where the 8086 has #BHE.
#include "pins_i8086_bus.h"
// QS0, #S1, #S0 and #TEST share A16-A19's port.
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

#include "pins_i8086_base.h"
#include "pwm_clock.h"

namespace debugger {
namespace i8086 {

// CLK free-runs from FlexPWM3, as the NMOS 8087 and the V20/V30 need, and
// the debugger holds each bus cycle in wait states with READY low.
struct PinsI8086 final : PinsI8086Base {
    // |fpu|: the D8087 board's 8087 shares the bus. Reset tells the 8088
    // and V20 from the 8086 and V30, and the NEC parts from Intel's.
    PinsI8086(bool fpu);

    // The clock runs on; READY holds the parked CPU.
    void idle() override;
#ifdef TUNE_CLOCK
    uint32_t clockInput() const override { return _clockInput; }
    bool setClockInput(uint32_t hz) override;
#endif

    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;

private:
    PwmClock<PIN_CLK> _clock;
#ifdef TUNE_CLOCK
    // CLK's rate, which K sets.
    uint32_t _clockInput;
#endif
    // The next cycle's T1, read ahead by completeCycle().
    Signals _latch;
    uint8_t _latchStatus;
    // When _latch showed its status, in CPU cycles.
    uint32_t _latchSeen;
    bool _latched = false;

    void resetPins() override;
    Signals *prepareCycle() override;
    Signals *completeCycle(Signals *s) override;
    void assertNmi() override;
    void negateNmi() override;
    void freezeQs() override;
    uint32_t clockCycles() const override;
    bool latchCycle();
    void applyChipClock();
    // Parks at the reset fetch, |bus8| as the CPU showed it there; false
    // if the CPU's first cycle is not that fetch.
    bool parkAtReset(Signals *s, bool bus8);
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
