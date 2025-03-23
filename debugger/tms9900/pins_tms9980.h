#ifndef __PINS_TMS9980_H__
#define __PINS_TMS9980_H__

#define PORT_DATA 6  /* GPIO6 */
#define DATA_gp 16   /* P6.16-P6.23 */
#define DATA_gm 0xFF /* P6.16-P6.23 */
#define DATA_vp 0    /* D7-D0 */
#define PIN_D7 19    /* P6.16 */
#define PIN_D6 18    /* P6.17 */
#define PIN_D5 14    /* P6.18 */
#define PIN_D4 15    /* P6.19 */
#define PIN_D3 40    /* P6.20 */
#define PIN_D2 41    /* P6.21 */
#define PIN_D1 17    /* P6.22 */
#define PIN_D0 16    /* P6.23 */
#define PORT_AL 6    /* GPIO6 */
#define AL_gp 24     /* P6.24-P6.31 */
#define AL_gm 0xFF   /* P6.24-P6.31 */
#define AL_vp 0      /* A13-A6 */
#define PIN_AL13 22  /* P6.24 */
#define PIN_AL12 23  /* P6.25 */
#define PIN_AL11 20  /* P6.26 */
#define PIN_AL10 21  /* P6.27 */
#define PIN_AL9 38   /* P6.28 */
#define PIN_AL8 39   /* P6.29 */
#define PIN_AL7 26   /* P6.30 */
#define PIN_AL6 27   /* P6.31 */
#define PORT_AM 7    /* GPIO7 */
#define AM_gp 0      /* P7.00-P7.03 */
#define AM_gm 0xF    /* P7.00-P7.03 */
#define AM_vp 8      /* A5-A2 */
#define PIN_AM5 10   /* P7.00 */
#define PIN_AM4 12   /* P7.01 */
#define PIN_AM3 11   /* P7.02 */
#define PIN_AM2 13   /* P7.03 */
#define PORT_AH 7    /* P7.16-P7.17 */
#define AH_gp 16     /* P7.16-P7.17 */
#define AH_gm 0x3    /* P7.16-P7.17 */
#define AH_vp 12     /* A1-A0 */
#define PIN_AH1 8    /* P7.16 */
#define PIN_AH0 7    /* P7.17 */
#define PORT_CNTL 9  /* GPIO9 */
#define CNTL_gp 4    /* P9.04-P9.07 */
#define CNTL_gm 0xF  /* P9.04-P9.07 */
#define CNTL_vp 0    /* CNTL0-CNTL3 */
#define PIN_MEMEN 2  /* P9.04 */
#define PIN_DBIN 3   /* P9.05 */
#define PIN_WE 4     /* P9.06 */
#define PIN_IAQ 33   /* P9.07 */
#define PIN_CRUCLK 0 /* P6.03 */
#define PIN_CRUIN 1  /* P6.02 */
#define PIN_CKIN 5   /* P9.08 */
#define PIN_PHI3 29  /* P9.31 */
#define PORT_INT 7   /* GPIO7 */
#define INT_gp 10    /* P7.10-P7.12 */
#define INT_gm 7     /* P7.10-P7.12 */
#define INT_vp 0     /* INT2-INT0 */
#define PIN_INT2 6   /* P7.10 */
#define PIN_INT1 9   /* P7.11 */
#define PIN_INT0 32  /* P7.12 */
#define PIN_HOLDA 28 /* P8.18 */
#define PIN_HOLD 30  /* P8.23 */

#include "pins_tms9900_base.h"

namespace debugger {
namespace tms9980 {

using tms9900::Signals;

struct PinsTms9980 final : tms9900::PinsTms9900Base {
    PinsTms9980();

    void resetPins() override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;

    void injectReads(const uint16_t *data, uint_fast8_t len) override;
    void captureWrites(uint16_t *buf, uint_fast8_t len) override;

private:
    Signals *resumeCycle(uint16_t pc = 0) override;
    Signals *prepareCycle() override;
    Signals *completeCycle(Signals *s) override;
};

}  // namespace tms9980
}  // namespace debugger
#endif /* __PINS_TMS9980_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
