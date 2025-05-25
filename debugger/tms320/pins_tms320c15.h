#ifndef __PINS_TMS320C15_H__
#define __PINS_TMS320C15_H__

#define PORT_DATA 6    /* GPIO6 */
#define DATA_gp 16     /* P6.16-P6.31 */
#define DATA_gm 0xFFFF /* P6.16-P6.31 */
#define DATA_vp 0      /* D0-D15 */
#define PIN_D0 19      /* P6.16 */
#define PIN_D1 18      /* P6.17 */
#define PIN_D2 14      /* P6.18 */
#define PIN_D3 15      /* P6.19 */
#define PIN_D4 40      /* P6.20 */
#define PIN_D5 41      /* P6.21 */
#define PIN_D6 17      /* P6.22 */
#define PIN_D7 16      /* P6.23 */
#define PIN_D8 22      /* P6.24 */
#define PIN_D9 23      /* P6.25 */
#define PIN_D10 20     /* P6.26 */
#define PIN_D11 21     /* P6.27 */
#define PIN_D12 38     /* P6.28 */
#define PIN_D13 39     /* P6.29 */
#define PIN_D14 26     /* P6.30 */
#define PIN_D15 27     /* P6.31 */
#define PORT_AL 7      /* GPIO7 */
#define AL_gp 0        /* P7.00-P7.03 */
#define AL_gm 0xF      /* P7.00-P7.03 */
#define AL_vp 0        /* A0-A3 */
#define PIN_AL0 10     /* P7.00 */
#define PIN_AL1 12     /* P7.01 */
#define PIN_AL2 11     /* P7.02 */
#define PIN_AL3 13     /* P7.03 */
#define PORT_AM 7      /* P7.16-P7.19 */
#define AM_gp 16       /* P7.16-P7.19 */
#define AM_gm 0xF      /* P7.16-P7.19 */
#define AM_vp 4        /* A4-A7 */
#define PIN_AM4 8      /* P7.16 */
#define PIN_AM5 7      /* P7.17 */
#define PIN_AM6 36     /* P7.18 */
#define PIN_AM7 37     /* P7.19 */
#define PORT_AH 9      /* GPIO9 */
#define AH_gp 4        /* P9.04-P9.07 */
#define AH_gm 0xF      /* P9.04-P9.07 */
#define AH_vp 8        /* A8-A11 */
#define PIN_AH8 2      /* P9.04 */
#define PIN_AH9 3      /* P9.05 */
#define PIN_AH10 4     /* P9.06 */
#define PIN_AH11 33    /* P9.07 */
#define PIN_BIO 1      /* P6.02 */
#define PIN_INT 5      /* P9.08 */
#define PIN_CLKIN 29   /* P9.31 */
#define PORT_CNTL 7    /* GPIO7 */
#define CNTL_gp 10     /* P7.10-P7.12 */
#define CNTL_gm 0x7    /* P7.10-P7.12 */
#define CNTL_vp 0      /* CNTL0-CNTL2 */
#define CNTL_MEM 0x1   /* CNTL0 */
#define CNTL_WE 0x2    /* CNTL1 */
#define CNTL_DEN 0x4   /* CNTL2 */
#define PIN_MEM 6      /* P7.10 */
#define PIN_WE 9       /* P7.11 */
#define PIN_DEN 32     /* P7.12 */
#define PIN_RS 28      /* P8.18 */
#define PIN_CLKOUT 30  /* P8.23 */

#include "pins_tms320.h"
#include "signals_tms320c15.h"

namespace debugger {
namespace tms320c15 {

struct PinsTms320C15 final : tms320::PinsTms320 {
    PinsTms320C15();

    void idle() override;
    bool step(bool show) override;
    void run() override;

    void printCycles() override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;
    void setBreakInst(uint32_t addr) const override;

    uint16_t injectRead(uint16_t data) override;
    uint16_t captureWrite() override;

private:
    void resetPins() override;
    bool rawStep();
    void loop();

    Signals *prepareCycle();
    Signals *completeCycle(Signals *s);

    void disassembleCycles();
};

}  // namespace tms320c15
}  // namespace debugger
#endif /* __PINS_TMS320C15_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
