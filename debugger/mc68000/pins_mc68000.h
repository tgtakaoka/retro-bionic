#ifndef __PINS_MC68000_H__
#define __PINS_MC68000_H__

#define PORT_D 6    /* GPIO6 */
#define D_gp 16     /* P6.16-P6.31 */
#define D_gm 0xFFFF /* P6.16-P6.31 */
#define D_vp 0      /* D0-D15 */
#define PIN_D0 19   /* P6.16 */
#define PIN_D1 18   /* P6.17 */
#define PIN_D2 14   /* P6.18 */
#define PIN_D3 15   /* P6.19 */
#define PIN_D4 40   /* P6.20 */
#define PIN_D5 41   /* P6.21 */
#define PIN_D6 17   /* P6.22 */
#define PIN_D7 16   /* P6.23 */
#define PIN_D8 22   /* P6.24 */
#define PIN_D9 23   /* P6.25 */
#define PIN_D10 20  /* P6.26 */
#define PIN_D11 21  /* P6.27 */
#define PIN_D12 38  /* P6.28 */
#define PIN_D13 39  /* P6.29 */
#define PIN_D14 26  /* P6.30 */
#define PIN_D15 27  /* P6.31 */
// A0-A23, FC2:FC0 and MODE share P30-P37 through four 74HCS153 selected
// by ASEL1:ASEL0: AL carries A0-A3 (00), A4-A7 (01), A12-A15 (10) and
// A8-A11 (11), AH A16-A19, A20-A23, FC2:FC0 with MODE on P37, and
// nothing. A0 is real only on an MC68HC001 strapped for 8 bits; the P
// and PS boards tie it low, the FN board pulls it down. One read of GPIO7
// takes both nibbles.
#define PORT_ADDR 7 /* GPIO7 */
#define PIN_AL0 10  /* P7.00 */
#define PIN_AL1 12  /* P7.01 */
#define PIN_AL2 11  /* P7.02 */
#define PIN_AL3 13  /* P7.03 */
#define PIN_AH0 8   /* P7.16 */
#define PIN_AH1 7   /* P7.17 */
#define PIN_AH2 36  /* P7.18 */
#define PIN_AH3 37  /* P7.19 */
// #IPL2-#IPL0 on P52-P50, one write of GPIO7.
#define PORT_IPL 7  /* GPIO7 */
#define IPL_gp 10   /* P7.10-P7.12 */
#define IPL_gm 7    /* P7.10-P7.12 */
#define IPL_vp 0    /* #IPL0-#IPL2 */
#define PIN_IPL0 6  /* P7.10 */
#define PIN_IPL1 9  /* P7.11 */
#define PIN_IPL2 32 /* P7.12 */
#define PIN_UDS PIN_P42
#define PIN_LDS PIN_P43
#define PIN_ASEL1 30 /* P8.23 */

#include "pins_mc68000_base.h"

namespace debugger {
namespace mc68000 {

// The MC68000P (DIP64), HD68HC000PS (SDIP64) and MC68000FN (PLCC68)
// boards, for the MC68000, MC68HC000, MC68HC001 and MC68010: a 16-bit
// bus, or an 8-bit one from an MC68HC001 with MODE low.
struct PinsMc68000 final : PinsMc68000Base {
    PinsMc68000();

private:
    // An MC68HC001 strapped for 8 bits: #LDS alone, A0 on the bus.
    bool _bus8 = false;

    void resetPins() override;
    void readStraps() override;
    void getAddr(Signals *s) const override;
    void getLanes(Signals *s) const override;
    void getData(Signals *s) const override;
    void outData(const Signals *s) const override;
    void inputMode() const override;
    void setIpl() const override;
};

}  // namespace mc68000
}  // namespace debugger
#endif /* __PINS_MC68000_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
