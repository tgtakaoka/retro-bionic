#ifndef __PINS_MC68HC05C0_H__
#define __PINS_MC68HC05C0_H__

#define ACIA_BASE 0xFFE0

#define PORT_AD 6      /* GPIO6 */
#define AD_gp 16       /* P6.16-P6.23 */
#define AD_gm 0xFF     /* P6.00-P6.07 */
#define AD_vp 0        /* AD0-AD7 */
#define PIN_AD0 19     /* P6.16 */
#define PIN_AD1 18     /* P6.17 */
#define PIN_AD2 14     /* P6.18 */
#define PIN_AD3 15     /* P6.19 */
#define PIN_AD4 40     /* P6.20 */
#define PIN_AD5 41     /* P6.21 */
#define PIN_AD6 17     /* P6.22 */
#define PIN_AD7 16     /* P6.23 */
#define PORT_ADDR 6    /* GPIO6 */
#define ADDR_gp 16     /* P6.16-P6.31 */
#define ADDR_gm 0xFFFF /* P6.16-P6.31 */
#define ADDR_vp 0      /* AD0-AD7:A8-A15 */
#define PIN_ADDR8 22   /* P6.24 */
#define PIN_ADDR9 23   /* P6.25 */
#define PIN_ADDR10 20  /* P6.26 */
#define PIN_ADDR11 21  /* P6.27 */
#define PIN_ADDR12 38  /* P6.28 */
#define PIN_ADDR13 39  /* P6.29 */
#define PIN_ADDR14 26  /* P6.30 */
#define PIN_ADDR15 27  /* P6.31 */
#define PORT_CNTL 9    /* GPIO9 */
#define CNTL_gp 4      /* P9.04-P4.07 */
#define CNTL_gm 0xF    /* P9.04-P9.07 */
#define CNTL_vp 0      /* CNTL0-CNTL3 */
#define PIN_AS 2       /* P9.04 */
#define PIN_WR 3       /* P9.05 */
#define PIN_LIR 4      /* P9.06 */
#define PIN_RD 33      /* P9.07 */
#define CNTL_AS 0x1    /* CNTL0 */
#define CNTL_RD 0x8    /* CNTL3 */
#define PIN_TDO 0      /* P6.03 */
#define PIN_RDI 1      /* P6.02 */
#define PIN_OSC1 5     /* P9.08 */
#define PIN_PB5 29     /* P9.31 */
#define PIN_IRQ 6      /* P7.10 */
#define PIN_PB4 32     /* P7.12 */
#define PIN_RESET 28   /* P8.18 */
#define PIN_PB0 31     /* P8.22 */
#define PIN_PB1 30     /* P8.23 */

#include "pins_mc6805.h"

namespace debugger {
namespace mc68hc05c0 {

struct PinsMc68HC05C0 final : mc6805::PinsMc6805 {
    PinsMc68HC05C0();

    void resetPins() override;
    void idle() override;

    bool is_internal(uint16_t addr) const override;

protected:
    mc6805::Signals *currCycle() const override;
    mc6805::Signals *rawPrepareCycle() const override;
    mc6805::Signals *prepareCycle() const override { return rawPrepareCycle(); }
    mc6805::Signals *completeCycle(mc6805::Signals *signals) const override;
};

}  // namespace mc68hc05c0
}  // namespace debugger

#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
