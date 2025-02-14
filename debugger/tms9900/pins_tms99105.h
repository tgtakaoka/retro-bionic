#ifndef __PINS_TMS99105_H__
#define __PINS_TMS99105_H__

#define PORT_ADDR 6    /* GPIO6 */
#define ADDR_gp 17     /* P6.17-P6.31 */
#define ADDR_gm 0x7FFF /* P6.17-P6.31 */
#define ADDR_vp 1      /* AD14-AD0 */
#define PORT_DATA 6    /* GPIO6 */
#define DATA_gp 16     /* P6.16-P6.31 */
#define DATA_gm 0xFFFF /* P6.16-P6.31 */
#define DATA_vp 0      /* D15-AD0 */
#define PIN_D15 19     /* P6.16 */
#define PIN_AD14 18    /* P6.17 */
#define PIN_AD13 14    /* P6.18 */
#define PIN_AD12 15    /* P6.19 */
#define PIN_AD11 40    /* P6.20 */
#define PIN_AD10 41    /* P6.21 */
#define PIN_AD9 17     /* P6.22 */
#define PIN_AD8 16     /* P6.23 */
#define PIN_AD7 22     /* P6.24 */
#define PIN_AD6 23     /* P6.25 */
#define PIN_AD5 20     /* P6.26 */
#define PIN_AD4 21     /* P6.27 */
#define PIN_AD3 38     /* P6.28 */
#define PIN_AD2 39     /* P6.29 */
#define PIN_AD1 26     /* P6.30 */
#define PIN_AD0 27     /* P6.31 */
#define PORT_BST 7     /* GPIO7 */
#define BST_gp 0       /* P7.00-P7.02 */
#define BST_gm 0x7     /* P7.00-P7.02 */
#define BST_vp 1       /* BST1-BST3 */
#define PIN_BST1 10    /* P7.00 */
#define PIN_BST2 12    /* P7.01 */
#define PIN_BST3 11    /* P7.02 */
#define PORT_IC 7      /* P7.16-P7.19 */
#define IC_gp 16       /* P7.16-P7.19 */
#define IC_gm 0xF      /* P7.16-P7.19 */
#define IC_vp 0        /* IC3-IC0 */
#define PIN_IC3 8      /* P7.16 */
#define PIN_IC2 7      /* P7.17 */
#define PIN_IC1 36     /* P7.18 */
#define PIN_IC0 37     /* P7.19 */
#define PORT_CNTL 9    /* GPIO9 */
#define CNTL_gp 4      /* P9.04-P9.06 */
#define CNTL_gm 0x7    /* P9.04-P9.06 */
#define CNTL_vp 0      /* CNTL0-CNTL2 */
#define PIN_MEM 2      /* P9.04 */
#define PIN_RW 3       /* P9.05 */
#define PIN_WE 4       /* P9.06 */
#define PIN_ALATCH 0   /* P6.03 */
#define PIN_RD 1       /* P6.02 */
#define PIN_CLKIN 5    /* P9.08 */
#define PIN_CLKOUT 29  /* P9.31 */
#define PIN_NMI 6      /* P7.10 */
#define PIN_INTREQ 9   /* P7.11 */
#define PIN_APP 32     /* P7.12 */
#define PIN_RESET 28   /* P8.18 */
#define PIN_HOLD 30    /* P8.23 */

#include "pins_tms9900.h"

namespace debugger {
namespace tms99105 {

struct PinsTms99105 final : tms9900::PinsTms9900 {
    PinsTms99105();

    void resetPins() override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;

    void injectReads(const uint16_t *data, uint8_t len);
    void captureReads(uint16_t *buf, uint8_t len) {
        captureCycles(buf, len, false);
    }
    void captureWrites(uint16_t *buf, uint8_t len) {
        captureCycles(buf, len, true);
    }

private:
    uint16_t _addr;

    tms9900::Signals *pauseCycle() override;
    tms9900::Signals *resumeCycle(uint16_t pc = 0) const override;
    tms9900::Signals *prepareCycle() const override;
    tms9900::Signals *completeCycle(tms9900::Signals *s) const override;

    void captureCycles(uint16_t *buf, uint8_t len, bool write);
    void checkCpuType();
};

}  // namespace tms99105
}  // namespace debugger
#endif /* __PINS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
