#ifndef __PINS_I80186_H__
#define __PINS_I80186_H__

// The BionicN80C186 and BionicN80C187 boards (schematics/n80c186): an
// N80C186XL, with an N80C187 on the second, in queue status mode.
#include "pins_i8086_bus.h"
#define PIN_RDQSMD 8 /* P7.16 P34, held low: queue status mode */
#define PIN_BHE 7    /* P7.17 P35 */
#define PIN_ALE2 36  /* P7.18 P36, ALE/QS0's FlexIO3 copy */
#define PIN_TEST 37  /* P7.19 P37, the BionicN80C187 board's 80C187 BUSY */
#define BHE_BIT 17   /* #BHE in PORT_AH */
// QS0, QS1 and #S0-#S2 in one read of GPIO9.
#define PORT_CNTL 9  /* GPIO9 */
#define CNTL_gp 4    /* P9.04-P9.08 */
#define CNTL_gm 0x1F /* P9.04-P9.08 */
#define CNTL_vp 0
#define PIN_QS0 2      /* P9.04 P40, ALE in normal mode */
#define PIN_QS1 3      /* P9.05 P41, #WR in normal mode */
#define PIN_S0 4       /* P9.06 P42 */
#define PIN_S1 33      /* P9.07 P43 */
#define PIN_S2 5       /* P9.08 P46 */
#define PIN_CLKOUT 0   /* P6.03 P44 */
#define PIN_DEN 1      /* P6.02 P45 */
#define PIN_X1 29      /* P9.31 P47 */
#define PIN_NMI 6      /* P7.10 P50 */
#define PIN_INT0 9     /* P7.11 P51 */
#define PIN_ARDY 32    /* P7.12 P52 */
#define PIN_RES 28     /* P8.18 P53 */
#define PIN_HOLD 31    /* P8.22 P54 */
#define PIN_HLDA 30    /* P8.23 P55 */

#if defined(I80186_NORMAL_BUS)
#error "N80C186: the normal bus mode (ALE, #RD, #WR) is not written yet"
#endif

#include "pins_i8086_base.h"

namespace debugger {
namespace i8086 {

// The 80C186XL is static: the debugger toggles X1 itself, one edge at a
// time, and a cycle lasts as long as it takes to serve.
struct PinsI80186 final : PinsI8086Base {
    // |npx|: the BionicN80C187 board's 80C187 drives TEST/BUSY.
    PinsI80186(bool npx);

    // The CPU waits unclocked.
    void idle() override {}
#ifdef TUNE_CLOCK
    bool setClockInput(uint32_t hz) override;
#endif

    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;

private:
    // CLKOUT periods clocked, each with its queue status sample.
    uint32_t _clocks = 0;
    // GPIO9 as read after the latest rising edge of CLKOUT.
    uint32_t _p9;

    void resetPins() override;
    Signals *prepareCycle() override;
    Signals *completeCycle(Signals *s) override;
    void assertNmi() override;
    void negateNmi() override;
    void freezeQs() override;
    uint32_t clockCycles() const override { return 1; }
    void fall();
    void rise();
    // Whether |s| accesses the Peripheral Control Block, which the CPU
    // serves itself.
    bool pcb(const Signals *s) const;
    // Whether |s| accesses the 80C187.
    bool npx(const Signals *s) const;
    bool parkAtReset(Signals *s);
};

}  // namespace i8086
}  // namespace debugger
#endif /* __PINS_I80186_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
