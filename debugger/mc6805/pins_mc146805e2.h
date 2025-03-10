#ifndef __PINS_MC146805E2_H__
#define __PINS_MC146805E2_H__

// To support ASSIST05
#define ACIA_BASE 0x17F8

#define PORT_B 6       /* GPIO6 */
#define B_gp 16        /* P6.16-P6.23 */
#define B_gm 0xFF      /* P6.16-P6.23 */
#define B_vp 0         /* B0-B7 */
#define PIN_B0 19      /* P6.16 */
#define PIN_B1 18      /* P6.17 */
#define PIN_B2 14      /* P6.18 */
#define PIN_B3 15      /* P6.19 */
#define PIN_B4 40      /* P6.20 */
#define PIN_B5 41      /* P6.21 */
#define PIN_B6 17      /* P6.22 */
#define PIN_B7 16      /* P6.23 */
#define PORT_ADDR 6    /* GPIO6 */
#define ADDR_gp 16     /* P6.16-P6.28 */
#define ADDR_gm 0x1FFF /* P6.16-P6.28 */
#define ADDR_vp 0      /* B0-B7:A8-A12 */
#define PIN_ADDR8 22   /* P6.24 */
#define PIN_ADDR9 23   /* P6.25 */
#define PIN_ADDR10 20  /* P6.26 */
#define PIN_ADDR11 21  /* P6.27 */
#define PIN_ADDR12 38  /* P6.28 */
#define PIN_PB6 26     /* P6.30 */
#define PIN_PB7 27     /* P6.31 */
#define PORT_CNTL 9    /* GPIO9 */
#define CNTL_gp 4      /* P9.04-P9.07 */
#define CNTL_gm 0xE    /* P9.05-P9.07 */
#define CNTL_vp 0      /* CNTL0-CNTL3 */
#define PIN_AS 2       /* P9.04 */
#define PIN_RW 3       /* P9.05 */
#define PIN_LI 4       /* P9.06 */
#define PIN_DS 33      /* P9.07 */
#define CNTL_RW 2      /* CNTL1 */
#define CNTL_LI 4      /* CNTL2 */
#define CNTL_DS 8      /* CNTL3 */
#define PIN_PB3 0      /* P6.03 */
#define PIN_PB2 1      /* P6.02 */
#define PIN_OSC1 5     /* P9.08 */
#define PIN_PB5 29     /* P9.31 */
#define PIN_TIMER 9    /* P7.11 */
#define PIN_PB4 32     /* P7.12 */
#define PIN_RESET 28   /* P8.18 */
#define PIN_PB0 31     /* P8.22 */
#define PIN_PB1 30     /* P8.23 */

#include "pins_mc6805.h"

namespace debugger {
namespace mc146805e2 {

using mc6805::Signals;

struct PinsMc146805E2 final : mc6805::PinsMc6805 {
    PinsMc146805E2();

    void idle() override;

    bool is_internal(uint16_t addr) const override { return addr < 0x80; }

protected:
    void resetCpu() override;
    Signals *currCycle(uint16_t pc = 0) const override;
    Signals *rawPrepareCycle() override;
    Signals *prepareCycle() override;
    Signals *completeCycle(Signals *signals) override;
};

}  // namespace mc146805e2
}  // namespace debugger

#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
