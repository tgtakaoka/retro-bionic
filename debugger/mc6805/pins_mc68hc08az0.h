#ifndef __PINS_MC68HC08AZ0_H__
#define __PINS_MC68HC08AZ0_H__

#define ACIA_BASE 0xFFE0

#define PORT_ED 6   /* GPIO6 */
#define ED_gp 16    /* P6.16-P6.23 */
#define ED_gm 0xFF  /* P6.16-P6.23 */
#define ED_vp 0     /* ED0-ED7 */
#define PIN_ED0 19  /* P6.16 */
#define PIN_ED1 18  /* P6.17 */
#define PIN_ED2 14  /* P6.18 */
#define PIN_ED3 15  /* P6.19 */
#define PIN_ED4 40  /* P6.20 */
#define PIN_ED5 41  /* P6.21 */
#define PIN_ED6 17  /* P6.22 */
#define PIN_ED7 16  /* P6.23 */
#define PORT_EAL 6  /* GPIO6 */
#define EAL_gp 24   /* P6.24-P6.31 */
#define EAL_gm 0xFF /* P6.24-P6.31 */
#define EAL_vp 0    /* EA0-EA7 */
#define PIN_EA0 22  /* P6.24 */
#define PIN_EA1 23  /* P6.25 */
#define PIN_EA2 20  /* P6.26 */
#define PIN_EA3 21  /* P6.27 */
#define PIN_EA4 38  /* P6.28 */
#define PIN_EA5 39  /* P6.29 */
#define PIN_EA6 26  /* P6.30 */
#define PIN_EA7 27  /* P6.31 */
#define PORT_EAM 7  /* GPIO7 */
#define EAM_gp 0    /* P7.00-P7.03 */
#define EAM_gm 0xF  /* P7.00-P7.03 */
#define EAM_vp 8    /* EA8-EA11 */
#define PIN_EA8 10  /* P7.00 */
#define PIN_EA9 12  /* P7.01 */
#define PIN_EA10 11 /* P7.02 */
#define PIN_EA11 13 /* P7.03 */
#define PORT_EAH 7  /* GPIO7 */
#define EAH_gp 16   /* P7.16-P7.19 */
#define EAH_gm 0xF  /* P7.16-P7.19 */
#define EAH_vp 12   /* EA12-EA15 */
#define PIN_EA12 8  /* P7.16 */
#define PIN_EA13 7  /* P7.17 */
#define PIN_EA14 36 /* P7.18 */
#define PIN_EA15 37 /* P7.19 */
#define PORT_CNTL 9 /* GPIO9 */
#define CNTL_gp 4   /* P9.04-P9.05 */
#define CNTL_gm 0x3 /* P9.04-P9.05 */
#define CNTL_vp 0   /* CNTL0-CNTL1 */
#define PIN_REB 2   /* P9.04 */
#define PIN_WEB 3   /* P9.05 */
#define CNTL_REB 1  /* CNTL0 */
#define CNTL_WEB 2  /* CNTL1 */
#define PIN_PTC0 4  /* P9.06 */
#define PIN_PTC1 33 /* P9.07 */
#define PIN_PTE0 0  /* P6.03 */
#define PIN_PTE1 1  /* P6.02 */
#define PIN_MCLK 5  /* P9.08 */
#define PIN_OSC1 29 /* P9.31 */
#define PIN_PTH0 9  /* P7.11 */
#define PIN_PTH1 32 /* P7.12 */
#define PIN_RST 28  /* P8.18 */
#define PIN_PTA0 31 /* P8.22 */
#define PIN_PTC3 30 /* P8.23 */

#include "pins_mc6805.h"

namespace debugger {
namespace mc68hc08az0 {

using mc6805::Signals;

struct PinsMc68HC08AZ0 final : mc6805::PinsMc6805 {
    PinsMc68HC08AZ0();

    void idle() override;

    bool is_internal(uint16_t addr) const override;

private:
    uint16_t _addr;
    uint_fast8_t _writes;

    void resetCpu() override;
    Signals *currCycle(uint16_t pc) const override;
    Signals *rawPrepareCycle() override;
    Signals *prepareCycle() override { return rawPrepareCycle(); }
    Signals *completeCycle(Signals *signals) override;
    bool rawStep() override;
    void loop() override;

    template <typename Inst>
    Inst *inst() const {
        return static_cast<Inst *>(_inst);
    }
};

}  // namespace mc68hc08az0
}  // namespace debugger

#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
