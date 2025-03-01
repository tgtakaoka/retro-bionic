#ifndef __PINS_MC6801_H__
#define __PINS_MC6801_H__

/**
 * For LILBUG's trace, Timer ouput (P21/PC1) must be connected to #NMI on board.
 */

/**
 * MC6801/MC6803
 *   MPU_MODE=2 Non-Multiplexed Mode (Internal RAM enabled)
 *   MPU_MODE=3 Non-Multiplexed Mode (Internal RAM disabled)
 * HD6301/HD6303
 *   MPU_MODE=2 Non-Multiplexed Mode (Internal RAM enabled)
 */
#define MPU_MODE 2

#define PORT_AD 6      /* GPIO6 */
#define AD_gp 16       /* P6.16 */
#define AD_gm 0xFF     /* P6.16-P6.23 */
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
#define ADDR_vp 0      /* A0-A15 */
#define PIN_ADDR8 22   /* P6.24 */
#define PIN_ADDR9 23   /* P6.25 */
#define PIN_ADDR10 20  /* P6.26 */
#define PIN_ADDR11 21  /* P6.27 */
#define PIN_ADDR12 38  /* P6.28 */
#define PIN_ADDR13 39  /* P6.29 */
#define PIN_ADDR14 26  /* P6.30 */
#define PIN_ADDR15 27  /* P6.31 */
#define PIN_AS 2       /* P9.04 */
#define PIN_PC0 4      /* P9.06 */
#define PIN_PC1 33     /* P9.07 */
#define PIN_SCITXD 0   /* P6.03 */
#define PIN_SCIRXD 1   /* P6.02 */
#define PIN_EXTAL 5    /* P9.08 */
#define PIN_E 29       /* P9.31 */
#define PIN_PC2 32     /* P7.12 */
#define PIN_RESET 28   /* P8.18 */
#define PIN_XTAL 30    /* P8.23 */

#include "pins_mc6800_base.h"
#include "signals_mc6801.h"

namespace debugger {
namespace mc6801 {

using mc6800::InstMc6800;
using mc6800::PinsMc6800Base;
using mc6800::RegsMc6800;

struct PinsMc6801 final : PinsMc6800Base {
    PinsMc6801();

    void resetPins() override;

    void idle() override;

protected:
    mc6800::Signals *cycle() override;
    mc6800::Signals *rawCycle() override;
    bool nonVmaAfteContextSave() const override { return !isHd63(); };

    bool isHd63() const;
};

}  // namespace mc6801
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
