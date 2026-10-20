#ifndef __PINS_MC9S12X_H__
#define __PINS_MC9S12X_H__

// The MC9S12X board: D0-D15 on P1x/P2x, ADDR0-ADDR22 on P30-P37 through
// four 74HCS153 selected by ASEL1:0 (P55/P54): 00 A0-A3/A16-A19, 01
// A4-A7/A20-A22, 10 A12-A15, 11 A8-A11, the low nibble on P30-P33.
#define PORT_DATA 6         /* GPIO6 */
#define DATA_gp 16          /* P6.16-P6.31 */
#define DATA_gm 0xFFFF      /* P6.16-P6.31 */
#define DATA_vp 0           /* D0-D15 */
#define PIN_D0 19           /* P6.16 */
#define PIN_D1 18           /* P6.17 */
#define PIN_D2 14           /* P6.18 */
#define PIN_D3 15           /* P6.19 */
#define PIN_D4 40           /* P6.20 */
#define PIN_D5 41           /* P6.21 */
#define PIN_D6 17           /* P6.22 */
#define PIN_D7 16           /* P6.23 */
#define PIN_D8 22           /* P6.24 */
#define PIN_D9 23           /* P6.25 */
#define PIN_D10 20          /* P6.26 */
#define PIN_D11 21          /* P6.27 */
#define PIN_D12 38          /* P6.28 */
#define PIN_D13 39          /* P6.29 */
#define PIN_D14 26          /* P6.30 */
#define PIN_D15 27          /* P6.31 */
#define PORT_MUXL 7         /* GPIO7 */
#define MUXL_gp 0           /* P7.00-P7.03 */
#define MUXL_gm 0xF         /* P7.00-P7.03 */
#define MUXL_vp 0           /* the mux outputs on P30-P33 */
#define PIN_MUX0 10         /* P7.00 P30 */
#define PIN_MUX1 12         /* P7.01 P31 */
#define PIN_MUX2 11         /* P7.02 P32 */
#define PIN_MUX3 13         /* P7.03 P33 */
#define PORT_MUXH 7         /* GPIO7 */
#define MUXH_gp 16          /* P7.16-P7.19 */
#define MUXH_gm 0xF         /* P7.16-P7.19 */
#define MUXH_vp 4           /* the mux outputs on P34-P37 */
#define PIN_MUX4 8          /* P7.16 P34 */
#define PIN_MUX5 7          /* P7.17 P35 */
#define PIN_MUX6 36         /* P7.18 P36 */
#define PIN_MUX7 37         /* P7.19 P37 */
#define PORT_ASEL 8         /* GPIO8 */
#define ASEL_gp 22          /* P8.22-P8.23 */
#define ASEL_gm 0x3         /* P8.22-P8.23 */
#define ASEL_vp 0           /* ASEL0-ASEL1 */
#define PIN_ASEL0 31        /* P8.22 P54 */
#define PIN_ASEL1 30        /* P8.23 P55 */
#define PIN_ECLK 2          /* P9.04 P40 */
#define PIN_RW 3            /* P9.05 P41 R/#W, #WE */
#define PIN_MODA 4          /* P9.06 P42 MODA, #RE */
#define PIN_RE PIN_MODA     /* P9.06 P42 */
#define PIN_MODB 33         /* P9.07 P43 MODB, TAGHI */
#define PIN_BKGD 0          /* P6.03 P44 BKGD, MODC */
#define PIN_ROMCTL 1        /* P6.02 P45 EWAIT, ROMCTL */
#define PIN_LSTRB 5         /* P9.08 P46 #LSTRB, #LDS, EROMCTL */
#define PIN_EXTAL 29        /* P9.31 P47 */
#define PIN_IRQ 6           /* P7.10 P50 */
#define PIN_XIRQ 9          /* P7.11 P51 */
#define PIN_XCLKS 32        /* P7.12 P52 XCLKS, ECLKX2 */
#define PIN_RESET 28        /* P8.18 P53 */
// The clock pins the shared code knows by the MC68HC12B board's names.
#define PIN_E PIN_ECLK

#include "pins_mc68hc12.h"

namespace debugger {
namespace mc68hc12 {

struct Mc9s12xInit;

/**
 * The MC9S12XDP512 (HCS12X) in special test mode on its own board: a
 * non-multiplexed 16-bit data bus, and the 23-bit address read through
 * the muxes, with the access source and the queue status after it.
 */
struct PinsMc9s12x final : PinsMc68hc12 {
    PinsMc9s12x(Mc9s12xInit &init);

private:
    // DATA carries the previous cycle's read data until ECLK falls.
    bool _driving = false;
    // The previous cycle, an internal read whose data IVD shows in this
    // cycle's ECLK-low phase.
    Signals *_ivd = nullptr;

    void resetPins() override;
    Signals *prepareCycle() override;
    Signals *completeCycle(Signals *s) override;
    void replay(Signals *begin, uint_fast8_t cycles) override;
};

}  // namespace mc68hc12
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
