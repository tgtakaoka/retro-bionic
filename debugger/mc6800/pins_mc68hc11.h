#ifndef __PINS_MC68HC11_H__
#define __PINS_MC68HC11_H__

#define PORT_AD 6        /* GPIO6 */
#define AD_gp 16         /* P6.16 */
#define AD_gm 0xFF       /* P6.16-P6.23 */
#define AD_vp 0          /* AD0-AD7 */
#define PIN_AD0 19       /* P6.16 */
#define PIN_AD1 18       /* P6.17 */
#define PIN_AD2 14       /* P6.18 */
#define PIN_AD3 15       /* P6.19 */
#define PIN_AD4 40       /* P6.20 */
#define PIN_AD5 41       /* P6.21 */
#define PIN_AD6 17       /* P6.22 */
#define PIN_AD7 16       /* P6.23 */
#define PORT_ADDR 6      /* GPIO6 */
#define ADDR_gp 16       /* P6.16-P6.31 */
#define ADDR_gm 0xFFFF   /* P6.16-P6.31 */
#define ADDR_vp 0        /* A0-A15 */
#define PIN_ADDR8 22     /* P6.24 */
#define PIN_ADDR9 23     /* P6.25 */
#define PIN_ADDR10 20    /* P6.26 */
#define PIN_ADDR11 21    /* P6.27 */
#define PIN_ADDR12 38    /* P6.28 */
#define PIN_ADDR13 39    /* P6.29 */
#define PIN_ADDR14 26    /* P6.30 */
#define PIN_ADDR15 27    /* P6.31 */
#define PIN_AS 2         /* P9.04 */
#define PIN_RW 3         /* P9.05 */
#define PIN_MODA 4       /* P9.06 */
#define PIN_LIR PIN_MODA /* P9.06 */
#define PIN_MODB 33      /* P9.07 */
#define PIN_SCITXD 0     /* P6.03 */
#define PIN_SCIRXD 1     /* P6.02 */
#define PIN_EXTAL 5      /* P9.08 */
#define PIN_E 29         /* P9.31 */
#define PIN_RESET 28     /* P8.18 */

#include "pins_mc6800_base.h"
#include "signals_mc68hc11.h"

namespace debugger {
namespace mc68hc11 {

struct Mc68hc11Init;

struct PinsMc68hc11 final : mc6800::PinsMc6800Base {
    PinsMc68hc11(Mc68hc11Init &init);

    void resetPins() override;
    // MC68HC11D is fully static, so we can stop clock safely.
    void idle() override {}

private:
    Mc68hc11Init &_init;

    mc6800::Signals *cycle() override;
    mc6800::Signals *rawCycle() override;

    void disassembleCycles() override;
};

}  // namespace mc68hc11
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
