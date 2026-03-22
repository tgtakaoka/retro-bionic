#ifndef __PINS_Z280_H__
#define __PINS_Z280_H__

#define PORT_AD 6    /* GPIO6 */
#define AD_gp 16     /* P6.16-P6.31 */
#define AD_gm 0xFFFF /* P6.16-P6.31 */
#define AD_vp 0      /* AD0-AD15 */
#define PIN_AD0 19   /* P6.16 */
#define PIN_AD1 18   /* P6.17 */
#define PIN_AD2 14   /* P6.18 */
#define PIN_AD3 15   /* P6.19 */
#define PIN_AD4 40   /* P6.20 */
#define PIN_AD5 41   /* P6.21 */
#define PIN_AD6 17   /* P6.22 */
#define PIN_AD7 16   /* P6.23 */
#define PIN_AD8 22   /* P6.24 */
#define PIN_AD9 23   /* P6.25 */
#define PIN_AD10 20  /* P6.26 */
#define PIN_AD11 21  /* P6.27 */
#define PIN_AD12 38  /* P6.28 */
#define PIN_AD13 39  /* P6.29 */
#define PIN_AD14 26  /* P6.30 */
#define PIN_AD15 27  /* P6.31 */
#define PORT_AL 7    /* GPIO7 */
#define AL_gp 0      /* P7.00-P7.03 */
#define AL_gm 0xF    /* P7.00-P7.03 */
#define AL_vp 16     /* A16-A19 */
#define PIN_ADR16 10 /* P7.00 */
#define PIN_ADR17 12 /* P7.01 */
#define PIN_ADR18 11 /* P7.02 */
#define PIN_ADR19 13 /* P7.03 */
#define PORT_AH 7    /* P7.16-P7.19 */
#define AH_gp 16     /* P7.16-P7.19 */
#define AH_gm 0xF    /* P7.16-P7.19 */
#define AH_vp 20     /* A20-A23 */
#define PIN_ADR20 8  /* P7.16 */
#define PIN_ADR21 7  /* P7.17 */
#define PIN_ADR22 36 /* P7.18 */
#define PIN_ADR23 37 /* P7.19 */
#define PIN_ST0 2    /* P9.04 */
#define PIN_ST1 3    /* P9.05 */
#define PIN_ST2 4    /* P9.06 */
#define PIN_ST3 33   /* P9.07 */
#define PORT_ST 9    /* GPIO9 */
#define ST_gp 4      /* P9.04-P9.07 */
#define ST_gm 0xF    /* P9.04-P9.07 */
#define ST_vp 0      /* ST0-ST3 */
#define PIN_CLK 0    /* P6.03 */
#define PIN_DS 1     /* P6.02 */
#define PIN_AS 5     /* P9.08 */
#define PIN_XTALI 29 /* P9.31 */
#define PIN_INTA 6   /* P7.10 */
#define PIN_NMI 9    /* P7.11 */
#define PIN_WAIT 32  /* P7.12 */
#define PIN_RESET 28 /* P8.18 */
#define PIN_RW 31    /* P8.22 */
#define PIN_BW 30    /* P8.23 */

#include "pins.h"
#include "signals_z280.h"

namespace debugger {
namespace z280 {

struct PinsZ280 final : Pins {
    PinsZ280();

    void idle() override;
    bool step(bool show) override;
    void run() override;

    void printCycles() override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;
    void setBreakInst(uint32_t addr) const override;

    uint16_t injectRead(uint16_t data);
    uint16_t captureWrite();

private:
    void resetPins() override;
    bool rawStep();
    void loop();

    Signals *prepareCycle();
    Signals *completeCycle(Signals *s);

    void disassembleCycles();
};

}  // namespace z280
}  // namespace debugger
#endif /* __PINS_Z280_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
