#ifndef __PINS_MC6802_H__
#define __PINS_MC6802_H__

#define PORT_D 6     /* GPIO6 */
#define D_gp 16      /* P6.16-P6.23 */
#define D_gm 0xFF    /* P6.16-P6.23 */
#define D_vp 0       /* D0-D7 */
#define PIN_D0 19    /* P6.16 */
#define PIN_D1 18    /* P6.17 */
#define PIN_D2 14    /* P6.18 */
#define PIN_D3 15    /* P6.19 */
#define PIN_D4 40    /* P6.20 */
#define PIN_D5 41    /* P6.21 */
#define PIN_D6 17    /* P6.22 */
#define PIN_D7 16    /* P6.23 */
#define PORT_AL 6    /* GPIO6 */
#define AL_gp 24     /* P6.24-P6.31 */
#define AL_gm 0xFF   /* P6.24-P6.31 */
#define AL_vp 0      /* A0-A7 */
#define PIN_AL0 22   /* P6.24 */
#define PIN_AL1 23   /* P6.25 */
#define PIN_AL2 20   /* P6.26 */
#define PIN_AL3 21   /* P6.27 */
#define PIN_AL4 38   /* P6.28 */
#define PIN_AL5 39   /* P6.29 */
#define PIN_AL6 26   /* P6.30 */
#define PIN_AL7 27   /* P6.31 */
#define PORT_AM 7    /* GPIO7 */
#define AM_gp 0      /* P7.00-P7.03 */
#define AM_gm 0xF    /* P7.12-P7.15 */
#define AM_vp 8      /* A8-A11 */
#define PIN_AM8 10   /* P7.00 */
#define PIN_AM9 12   /* P7.01 */
#define PIN_AM10 11  /* P7.02 */
#define PIN_AM11 13  /* P7.03 */
#define PORT_AH 7    /* P7.16-P7.19 */
#define AH_gp 16     /* P7.16-P7.19 */
#define AH_gm 0xF    /* P7.16-P7.19 */
#define AH_vp 12     /* A12-A15 */
#define PIN_AH12 8   /* P7.16 */
#define PIN_AH13 7   /* P7.17 */
#define PIN_AH14 36  /* P7.18 */
#define PIN_AH15 37  /* P7.19 */
#define PIN_VMA 2    /* P9.04 */
#define PIN_RW 3     /* P9.05 */
#define PIN_BA 33    /* P9.07 */
#define PIN_EXTAL 5  /* P9.08 */
#define PIN_E 29     /* P9.31 */
#define PIN_MR 32    /* P7.12 */
#define PIN_RESET 28 /* P8.18 */
#define PIN_RE 31    /* P8.22 */
#define PIN_HALT 30  /* P8.23 */

#include "pins_mc6800.h"
#include "regs_mc6802.h"
#include "signals_mc6800.h"

namespace debugger {
namespace mc6802 {

using mc6800::InstMc6800;
using mc6800::PinsMc6800;
using mc6800::Signals;

struct PinsMc6802 final : PinsMc6800 {
    PinsMc6802(RegsMc6802 &regs, InstMc6800 &inst, const Mems &mems, Devs &devs)
        : PinsMc6800(regs, inst, mems, devs) {}

    void reset() override;

protected:
    Signals *cycle() override;
    Signals *rawCycle() override;
};

}  // namespace mc6802
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
