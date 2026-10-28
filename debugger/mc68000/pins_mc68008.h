#ifndef __PINS_MC68008_H__
#define __PINS_MC68008_H__

// D0-D7 on P10-P17.
#define PORT_D 6    /* GPIO6 */
#define D_gp 16     /* P6.16-P6.23 */
#define D_gm 0xFF   /* P6.16-P6.23 */
#define D_vp 0      /* D0-D7 */
#define PIN_D0 19   /* P6.16 */
#define PIN_D1 18   /* P6.17 */
#define PIN_D2 14   /* P6.18 */
#define PIN_D3 15   /* P6.19 */
#define PIN_D4 40   /* P6.20 */
#define PIN_D5 41   /* P6.21 */
#define PIN_D6 17   /* P6.22 */
#define PIN_D7 16   /* P6.23 */
// A0-A7 direct on P20-P27.
#define PORT_AL 6   /* GPIO6 */
#define AL_gp 24    /* P6.24-P6.31 */
#define AL_gm 0xFF  /* P6.24-P6.31 */
#define AL_vp 0     /* A0-A7 */
#define PIN_AL0 22  /* P6.24 */
#define PIN_AL1 23  /* P6.25 */
#define PIN_AL2 20  /* P6.26 */
#define PIN_AL3 21  /* P6.27 */
#define PIN_AL4 38  /* P6.28 */
#define PIN_AL5 39  /* P6.29 */
#define PIN_AL6 26  /* P6.30 */
#define PIN_AL7 27  /* P6.31 */
// A8-A15 share P30-P33 through a 74AHCT157 selected by ASEL (P54): 0
// A8-A11, 1 A12-A15. P34-P37 carry A16-A19 direct on the P board; on the
// FN board a second 157 gives 0 A16-A19, 1 A20-A21. FC2:FC0 sit on
// P50-P52 in the same GPIO7, so one read takes them all.
#define PORT_ADDR 7 /* GPIO7 */
#define PIN_AM0 10  /* P7.00 */
#define PIN_AM1 12  /* P7.01 */
#define PIN_AM2 11  /* P7.02 */
#define PIN_AM3 13  /* P7.03 */
#define PIN_AH0 8   /* P7.16 */
#define PIN_AH1 7   /* P7.17 */
#define PIN_AH2 36  /* P7.18 */
#define PIN_AH3 37  /* P7.19 */
#define PIN_FC0 6   /* P7.10 */
#define PIN_FC1 9   /* P7.11 */
#define PIN_FC2 32  /* P7.12 */
#define PIN_IPL20 PIN_P42 /* #IPL2 and #IPL0 tied */
#define PIN_DS PIN_P43
#define PIN_ASEL PIN_ASEL0
#define PIN_IPL1 30 /* P8.23 */

#include "pins_mc68000_base.h"

namespace debugger {
namespace mc68000 {

// The MC68008P (DIP48, 20 address lines) and MC68008FN (PLCC52, 22) boards,
// told apart at reset by whether P34-P35 follow ASEL.
struct PinsMc68008 final : PinsMc68000Base {
    PinsMc68008();

private:
    // The FN board's 22 address lines, rather than the P board's 20.
    bool _fn = false;

    void resetPins() override;
    void detectBoard() override;
    void getAddr(Signals *s) const override;
    void getData(Signals *s) const override;
    void outData(const Signals *s) const override;
    void inputMode() const override;
    void setIpl() const override;
};

}  // namespace mc68000
}  // namespace debugger
#endif /* __PINS_MC68008_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
