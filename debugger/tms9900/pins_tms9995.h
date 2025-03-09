#ifndef __PINS_TMS9995_H__
#define __PINS_TMS9995_H__

#define PORT_DATA 6   /* GPIO6 */
#define DATA_gp 16    /* P6.16-P6.23 */
#define DATA_gm 0xFF  /* P6.16-P6.23 */
#define DATA_vp 0     /* D7-D0 */
#define PIN_D7 19     /* P6.16 */
#define PIN_D6 18     /* P6.17 */
#define PIN_D5 14     /* P6.18 */
#define PIN_D4 15     /* P6.19 */
#define PIN_D3 40     /* P6.20 */
#define PIN_D2 41     /* P6.21 */
#define PIN_D1 17     /* P6.22 */
#define PIN_D0 16     /* P6.23 */
#define PORT_AL 6     /* GPIO6 */
#define AL_gp 24      /* P6.24-P6.31 */
#define AL_gm 0xFF    /* P6.24-P6.31 */
#define AL_vp 0       /* A15-A8 */
#define PIN_AL15 22   /* P6.24 */
#define PIN_AL14 23   /* P6.25 */
#define PIN_AL13 20   /* P6.26 */
#define PIN_AL12 21   /* P6.27 */
#define PIN_AL11 38   /* P6.28 */
#define PIN_AL10 39   /* P6.29 */
#define PIN_AL9 26    /* P6.30 */
#define PIN_AL8 27    /* P6.31 */
#define PORT_AM 7     /* GPIO7 */
#define AM_gp 0       /* P7.00-P7.03 */
#define AM_gm 0xF     /* P7.00-P7.03 */
#define AM_vp 8       /* A7-A4 */
#define PIN_AM7 10    /* P7.00 */
#define PIN_AM6 12    /* P7.01 */
#define PIN_AM5 11    /* P7.02 */
#define PIN_AM4 13    /* P7.03 */
#define PORT_AH 7     /* P7.16-P7.17 */
#define AH_gp 16      /* P7.16-P7.17 */
#define AH_gm 0xF     /* P7.16-P7.17 */
#define AH_vp 12      /* A3-A0 */
#define PIN_AH3 8     /* P7.16 */
#define PIN_AH2 7     /* P7.17 */
#define PIN_AH1 36    /* P7.16 */
#define PIN_AH0 37    /* P7.17 */
#define PORT_CNTL 9   /* GPIO9 */
#define CNTL_gp 4     /* P9.04-P9.07 */
#define CNTL_gm 0xF   /* P9.04-P9.07 */
#define CNTL_vp 0     /* CNTL0-CNTL3 */
#define PIN_MEMEN 2   /* P9.04 */
#define PIN_DBIN 3    /* P9.05 */
#define PIN_WE 4      /* P9.06 */
#define PIN_IAQ 33    /* P9.07 */
#define PIN_CRUIN 1   /* P6.02 */
#define PIN_CLKIN 5   /* P9.08 */
#define PIN_CLKOUT 29 /* P9.31 */
#define PIN_NMI 6     /* P7.10 */
#define PIN_INT1 9    /* P7.11 */
#define PIN_INT4 32   /* P7.12 */
#define PIN_RESET 28  /* P8.18 */
#define PIN_HOLD 30   /* P8.23 */

#include "pins_tms9900.h"

namespace debugger {
namespace tms9995 {

struct PinsTms9995 final : tms9900::PinsTms9900 {
    PinsTms9995();

    void resetPins() override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;

    void injectReads(const uint16_t *data, uint_fast8_t len) override;
    void captureWrites(uint16_t *buf, uint_fast8_t len) override;

    bool is_internal(uint16_t addr) const;
    uint8_t internal_read(uint16_t addr);
    void internal_write(uint16_t addr, uint8_t data);
    uint16_t internal_read16(uint16_t addr);
    void internal_write16(uint16_t addr, uint16_t data);

private:
    void cycle();
    void inject(uint8_t data);
    uint8_t capture();
    tms9900::Signals *prepareCycle() const override;
    tms9900::Signals *completeCycle(tms9900::Signals *s) const override;
    tms9900::Signals *resumeCycle(uint16_t pc = 0) const override;
};

}  // namespace tms9995
}  // namespace debugger
#endif /* __PINS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
