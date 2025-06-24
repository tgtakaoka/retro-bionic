#ifndef __PINS_TMS370CX5X_H__
#define __PINS_TMS370CX5X_H__

#define PORT_DATA 6   /* GPIO6 */
#define DATA_gp 16    /* P6.16-P6.23 */
#define DATA_gm 0xFF  /* P6.16-P6.23 */
#define DATA_vp 0     /* DATA0-DATA7 */
#define PIN_DATA0 19  /* P6.16 */
#define PIN_DATA1 18  /* P6.17 */
#define PIN_DATA2 14  /* P6.18 */
#define PIN_DATA3 15  /* P6.19 */
#define PIN_DATA4 40  /* P6.20 */
#define PIN_DATA5 41  /* P6.21 */
#define PIN_DATA6 17  /* P6.22 */
#define PIN_DATA7 16  /* P6.23 */
#define PORT_ADRL 6   /* GPIO6 */
#define ADRL_gp 24    /* P6.24-P6.31 */
#define ADRL_gm 0xFF  /* P6.24-P6.31 */
#define ADRL_vp 0     /* ADDR0-ADDR7 */
#define PIN_ADDR0 22  /* P6.24 */
#define PIN_ADDR1 23  /* P6.25 */
#define PIN_ADDR2 20  /* P6.26 */
#define PIN_ADDR3 21  /* P6.27 */
#define PIN_ADDR4 38  /* P6.28 */
#define PIN_ADDR5 39  /* P6.29 */
#define PIN_ADDR6 26  /* P6.30 */
#define PIN_ADDR7 27  /* P6.31 */
#define PORT_ADRM 7   /* GPIO7 */
#define ADRM_gp 0     /* P7.00-P7.03 */
#define ADRM_gm 0xF   /* P7.00-P7.03 */
#define ADRM_vp 8     /* ADDR8-ADDR11 */
#define PIN_ADDR8 10  /* P7.00 */
#define PIN_ADDR9 12  /* P7.01 */
#define PIN_ADDR10 11 /* P7.02 */
#define PIN_ADDR11 13 /* P7.03 */
#define PORT_ADRH 7   /* GPIO7 */
#define ADRH_gp 16    /* P7.16-P7.19 */
#define ADRH_gm 0xF   /* P7.16-P7.19 */
#define ADRH_vp 12    /* ADDR12-ADDR15 */
#define PIN_ADDR12 8  /* P7.16 */
#define PIN_ADDR13 7  /* P7.17 */
#define PIN_ADDR14 36 /* P7.18 */
#define PIN_ADDR15 37 /* P7.19 */
#define PORT_CNTL 9   /* GPIO9 */
#define CNTL_gp 4     /* P9.04-P9.07 */
#define CNTL_gm 0x7   /* P9.04-P9.07 */
#define CNTL_vp 0     /* CNTL0-CNTL2 */
#define PIN_EDS 2     /* P9.04 */
#define PIN_RW 3      /* P9.05 */
#define PIN_OCF 4     /* P9.06 */
#define PIN_SCITXD 0  /* P6.03 */
#define PIN_SCIRXD 1  /* P6.02 */
#define PIN_CLKIN 5   /* P9.08 */
#define PIN_SYSCLK 29 /* P9.31 */
#define PIN_INT1 6    /* P7.10 */
#define PIN_INT2 9    /* P7.11 */
#define PIN_INT3 32   /* P7.12 */
#define PIN_RESET 28  /* P8.18 */
#define PIN_WAIT 31   /* P8.22 */

#include "pins_tms370.h"
#include "signals_tms370.h"

namespace debugger {
namespace tms370cx5x {

using tms370::Signals;

struct PinsTms370Cx5x final : tms370::PinsTms370 {
    PinsTms370Cx5x();

    void resetPins() override;
    void idle() override;
    bool step(bool show) override;
    void run() override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;

    void injectReads(const uint8_t *data, uint_fast8_t len) override;
    uint16_t captureWrites(const uint8_t *data, uint_fast8_t len, uint8_t *buf,
            uint_fast8_t max) override;

private:
    Signals *prepareCycle();
    Signals *completeCycle(Signals *s);

    void loop();
    bool rawStep();
    void pauseCpu();
    void resumeCpu();
    Signals *resetCpu();
};

}  // namespace tms370cx5x
}  // namespace debugger
#endif /* __PINS_TMS370CX5X_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
