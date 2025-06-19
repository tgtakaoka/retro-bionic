#ifndef __PINS_I8096_H__
#define __PINS_I8096_H__

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
#define PIN_RESET 28    /* P8.18 */
#define PIN_READY 31    /* P8.22 */
#define PIN_EXTINT 30   /* P8.23 */

#include "pins.h"

namespace debugger {
namespace i8096 {

struct PinsI8096 : Pins {
    uint16_t injectRead(uint8_t data) { return injectReads(&data, 1); }
    virtual uint16_t injectReads(
            const uint8_t *data, uint_fast8_t len, bool idle = false) = 0;
    virtual uint16_t captureWrites(uint8_t *data, uint_fast8_t len) = 0;
};

}  // namespace i8096
}  // namespace debugger
#endif /* __PINS_I8096_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
