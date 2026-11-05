#ifndef __PINS_I80286_H__
#define __PINS_I80286_H__

// The BionicN80C286 board (schematics/n80c286): an N80C286 whose CLK the
// Teensy toggles, A0-A23 through four 74HCS153 and HOLD, NMI and INTR on
// a 74HCT259. D0-D15 are AD0-AD15's pins.
#include "pins_i8086_bus.h"
// The muxes' outputs, P30-P33 and P34-P37, in one read of GPIO7.
#define PORT_MUX 7  /* GPIO7 */
#define PIN_MUX4 8  /* P7.16 P34 */
#define PIN_MUX5 7  /* P7.17 P35 */
#define PIN_MUX6 36 /* P7.18 P36 */
#define PIN_MUX7 37 /* P7.19 P37 */
#define MUXL_BIT 0  /* P30-P33 in PORT_MUX */
#define MUXH_BIT 16 /* P34-P37 in PORT_MUX */
// #S0, #S1, M/#IO, #BHE and COD/#INTA in one read of GPIO9.
#define PORT_CNTL 9  /* GPIO9 */
#define CNTL_gp 4    /* P9.04-P9.08 */
#define CNTL_gm 0x1F /* P9.04-P9.08 */
#define CNTL_vp 0
#define PIN_S0 2        /* P9.04 P40 */
#define PIN_S1 3        /* P9.05 P41 */
#define PIN_MIO 4       /* P9.06 P42 */
#define PIN_BHE 33      /* P9.07 P43 */
#define PIN_CODINTA 5   /* P9.08 P46 */
#define PIN_READY 0     /* P6.03 P44 */
#define PIN_LATCH_D 1   /* P6.02 P45, the 74HCT259's D */
#define PIN_CLK 29      /* P9.31 P47 */
#define PIN_LATCH_LE 32 /* P7.12 P52, the 74HCT259's #LE */
#define PIN_RESET 28    /* P8.18 P53 */
#define PIN_ASEL0 31    /* P8.22 P54, the muxes' and the 74HCT259's A0 */
#define PIN_ASEL1 30    /* P8.23 P55, the muxes' and the 74HCT259's A1 */

#include "pins_i8086_base.h"

namespace debugger {
namespace i8086 {

// The 80C286 is static: the debugger toggles CLK itself and stops it low
// in a phase 2, the only place Intel lets it stop.
struct PinsI80286 final : PinsI8086Base {
    PinsI80286();

    // The CPU waits unclocked.
    void idle() override {}

    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;

private:
    // #S0, #S1, M/#IO, #BHE and COD/#INTA as read in the latest phase 1.
    uint32_t _p9;

    void resetPins() override;
    Signals *prepareCycle() override;
    Signals *completeCycle(Signals *s) override;
    void assertNmi() override;
    void negateNmi() override;
    // No queue status to freeze.
    void freezeQs() override {}
    uint32_t clockCycles() const override { return 2; }
    // One processor clock from a phase 2: phase 1 starts at a falling
    // edge, the status is read, and the clock stops low in phase 2.
    void clock();
    bool syncPhase();
    bool parkAtReset(Signals *s);
};

}  // namespace i8086
}  // namespace debugger
#endif /* __PINS_I80286_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
