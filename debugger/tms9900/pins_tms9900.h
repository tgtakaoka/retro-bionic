#ifndef __PINS_TMS9900_H__
#define __PINS_TMS9900_H__

#define PORT_DATA 6    /* GPIO6 */
#define DATA_gp 16     /* P6.16-P6.31 */
#define DATA_gm 0xFFFF /* P6.16-P6.31 */
#define DATA_vp 0      /* D15-D0 */
#define PIN_D15 19     /* P6.16 */
#define PIN_D14 18     /* P6.17 */
#define PIN_D13 14     /* P6.18 */
#define PIN_D12 15     /* P6.19 */
#define PIN_D11 40     /* P6.20 */
#define PIN_D10 41     /* P6.21 */
#define PIN_D9 17      /* P6.22 */
#define PIN_D8 16      /* P6.23 */
#define PIN_D7 22      /* P6.24 */
#define PIN_D6 23      /* P6.25 */
#define PIN_D5 20      /* P6.26 */
#define PIN_D4 21      /* P6.27 */
#define PIN_D3 38      /* P6.28 */
#define PIN_D2 39      /* P6.29 */
#define PIN_D1 26      /* P6.30 */
#define PIN_D0 27      /* P6.31 */
#define PORT_ADR0 7    /* GPIO7 */
#define ADR0_gp 1      /* P7.01-P7.03 */
#define ADR0_gm 0x7    /* P7.01-P7.03 */
#define ADR0_vp 1      /* A14-A12 */
#define PORT_ADR1 7    /* GPIO7 */
#define ADR1_gp 16     /* P7.16-P7.19 */
#define ADR1_gm 0xF    /* P7.16-P7.19 */
#define ADR1_vp 4      /* A11-A8 */
#define PORT_ADR2 7    /* GPIO7 */
#define ADR2_gp 0      /* P7.00-P7.03 */
#define ADR2_gm 0xF    /* P7.00-P7.03 */
#define ADR2_vp 8      /* A7-A4 */
#define PORT_ADR3 7    /* GPIO7 */
#define ADR3_gp 16     /* P7.16-P7.19 */
#define ADR3_gm 0xF    /* P7.16-P7.19 */
#define ADR3_vp 12     /* A3-A0 */
#define PIN_CRUOUT 10  /* P7.00 */
#define PIN_ADR14 12   /* P7.01 */
#define PIN_ADR13 11   /* P7.02 */
#define PIN_ADR12 13   /* P7.03 */
#define PIN_ADR11 8    /* P7.16 */
#define PIN_ADR10 7    /* P7.17 */
#define PIN_ADR9 36    /* P7.18 */
#define PIN_ADR8 37    /* P7.19 */
#define PORT_CNTL 9    /* GPIO9 */
#define CNTL_gp 4      /* P9.04-P9.07 */
#define CNTL_gm 0xF    /* P9.04-P9.07 */
#define CNTL_vp 0      /* CNTL0-CNTL3 */
#define PIN_MEMEN 2    /* P9.04 */
#define PIN_DBIN 3     /* P9.05 */
#define PIN_WE 4       /* P9.06 */
#define PIN_IAQ 33     /* P9.07 */
#define PIN_CRUCLK 0   /* P6.03 */
#define PIN_CRUIN 1    /* P6.02 */
#define PIN_OUTCLK 5   /* P9.08 */
#define PIN_PHICLK 29  /* P9.31 */
#define PORT_SEL 7     /* GPIO7 */
#define SEL_gp 10      /* P7.10-P7.12 */
#define SEL_gm 0x7     /* P7.10-P7.12 */
#define SEL_vp 0       /* SEL0-SEL2 */
#define PIN_SEL0 6     /* P7.10 */
#define PIN_SEL1 9     /* P7.11 */
#define PIN_SEL2 32    /* P7.12 */
#define PIN_RESET 28   /* P8.18 */
#define PIN_DATA 30    /* P8.23 */

#define SEL_LOAD 0
#define SEL_INTREQ 1
#define SEL_HOLD 3
#define SEL_IC3 4
#define SEL_IC2 5
#define SEL_IC1 6
#define SEL_IC0 7

#include "pins_tms9900_base.h"

namespace debugger {
namespace tms9900 {

using tms9900::Signals;

struct PinsTms9900 final : tms9900::PinsTms9900Base {
    PinsTms9900();

    void resetPins() override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;

    void injectReads(const uint16_t *data, uint_fast8_t len) override;
    void captureWrites(uint16_t *buf, uint_fast8_t len) override;

private:
    uint16_t _addr;

    Signals *resumeCycle(uint16_t pc = 0) override;
    Signals *prepareCycle() override;
    Signals *completeCycle(tms9900::Signals *s) override;

    void checkCpuType();
};

}  // namespace tms9900
}  // namespace debugger
#endif /* __PINS_TMS9900_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
