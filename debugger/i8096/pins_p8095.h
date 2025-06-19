#ifndef __PINS_P8095_H__
#define __PINS_P8095_H__

#define PORT_DATA 6     /* GPIO6 */
#define DATA_gp 16      /* P6.16-P6.31 */
#define DATA_gm 0xFF    /* P6.16-P6.23 */
#define DATA_vp 0       /* D0-D7 */
#define PORT_AD 6       /* GPIO6 */
#define AD_gp 16        /* P6.16-P6.31 */
#define AD_gm 0xFFFF    /* P6.16-P6.31 */
#define AD_vp 0         /* A0-A15 */
#define PIN_AD0 19      /* P6.16 */
#define PIN_AD1 18      /* P6.17 */
#define PIN_AD2 14      /* P6.18 */
#define PIN_AD3 15      /* P6.19 */
#define PIN_AD4 40      /* P6.20 */
#define PIN_AD5 41      /* P6.21 */
#define PIN_AD6 17      /* P6.22 */
#define PIN_AD7 16      /* P6.23 */
#define PIN_AD8 22      /* P6.24 */
#define PIN_AD9 23      /* P6.25 */
#define PIN_AD10 20     /* P6.26 */
#define PIN_AD11 21     /* P6.27 */
#define PIN_AD12 38     /* P6.28 */
#define PIN_AD13 39     /* P6.29 */
#define PIN_AD14 26     /* P6.30 */
#define PIN_AD15 27     /* P6.31 */
#define PORT_HSO 7      /* GPIO7 */
#define HSO_gp 0        /* P7.00-P7.03 */
#define HSO_gm 0xF      /* P7.00-P7.03 */
#define HSO_vp 0        /* A0-A3 */
#define PIN_HSO0 10     /* P7.00 */
#define PIN_HSO1 12     /* P7.01 */
#define PIN_HSO2 11     /* P7.02 */
#define PIN_HSO3 13     /* P7.03 */
#define PORT_HSI 7      /* P7.16-P7.19 */
#define HSI_gp 16       /* P7.16-P7.19 */
#define HSI_gm 0xF      /* P7.16-P7.19 */
#define HSI_vp 4        /* A4-A7 */
#define PIN_HSI0 8      /* P7.16 */
#define PIN_HSI1 7      /* P7.17 */
#define PIN_HSI2 36     /* P7.18 */
#define PIN_HSI3 37     /* P7.19 */
#define PORT_CNTL 9     /* GPIO9 */
#define CNTL_gp 4       /* P9.04-P9.07 */
#define CNTL_gm 0xF     /* P9.04-P9.07 */
#define CNTL_vp 0       /* CNTL0-CNTL3 */
#define PIN_ADV 2       /* P9.04 */
#define PIN_RD 3        /* P9.05 */
#define PIN_WR 4        /* P9.06 */
#define PIN_BHE 33      /* P9.07 */
#define CNTL_ADV 0x1    /* CNTL0 */
#define CNTL_RD 0x2     /* CNTL1 */
#define CNTL_WR 0x4     /* CNTL2 */
#define CNTL_BHE 0x8    /* CNTL3 */
#define CNTL_FETCH 0x10 /* CNTL4 */
#define PIN_TXD 0       /* P6.03 */
#define PIN_RXD 1       /* P6.02 */
#define PIN_PWM 5       /* P9.08 */
#define PIN_XTAL1 29    /* P9.31 */
#define PIN_ACH4 6      /* P7.10 */
#define PIN_ACH5 9      /* P7.11 */
#define PIN_ACH6 32     /* P7.12 */
#define PIN_RESET 28    /* P8.18 */
#define PIN_READY 31    /* P8.22 */
#define PIN_EXTINT 30   /* P8.23 */
// #define PIN_ACH7 35   /* P7.28 */

#include "inst_i8096.h"
#include "pins_i8096.h"
#include "signals_p8095.h"

namespace debugger {
namespace p8095 {

struct PinsP8095 final : i8096::PinsI8096 {
    PinsP8095();

    void idle() override;
    bool step(bool show) override;
    void run() override;

    void printCycles() override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;
    void setBreakInst(uint32_t addr) const override;

    uint16_t injectReads(const uint8_t *data, uint_fast8_t len) override;
    uint16_t captureWrites(uint8_t *data, uint_fast8_t len) override;

private:
    i8096::InstI8096 *_inst;
    uint16_t _addr;

    void resetPins() override;
    bool rawStep(bool step = false);
    void loop();

    Signals *prepareCycle();
    Signals *completeCycle(Signals *s);

    Signals *jumpHere(uint_fast8_t len = 4);

    void disassembleCycles();
};

}  // namespace p8095
}  // namespace debugger
#endif /* __PINS_P8095_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
