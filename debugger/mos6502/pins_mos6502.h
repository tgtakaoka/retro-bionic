#ifndef __PINS_MOS6502_H__
#define __PINS_MOS6502_H__

#define PORT_D 6      /* GPIO6 */
#define D_gp 16       /* P6.16-P6.23 */
#define D_gm 0xFF     /* P6.16-P6.23 */
#define D_vp 0        /* D0-D7 */
#define PIN_D0 19     /* P6.16 */
#define PIN_D1 18     /* P6.17 */
#define PIN_D2 14     /* P6.18 */
#define PIN_D3 15     /* P6.19 */
#define PIN_D4 40     /* P6.20 */
#define PIN_D5 41     /* P6.21 */
#define PIN_D6 17     /* P6.22 */
#define PIN_D7 16     /* P6.23 */
#define PORT_AL 6     /* GPIO6 */
#define AL_gp 24      /* P6.24-P6.31 */
#define AL_gm 0xFF    /* P6.24-P6.31 */
#define AL_vp 0       /* A0-A7 */
#define PIN_AL0 22    /* P6.24 */
#define PIN_AL1 23    /* P6.25 */
#define PIN_AL2 20    /* P6.26 */
#define PIN_AL3 21    /* P6.27 */
#define PIN_AL4 38    /* P6.28 */
#define PIN_AL5 39    /* P6.29 */
#define PIN_AL6 26    /* P6.30 */
#define PIN_AL7 27    /* P6.31 */
#define PORT_AM 7     /* GPIO7 */
#define AM_gp 0       /* P7.00-P7.03 */
#define AM_gm 0xF     /* P7.12-P7.15 */
#define AM_vp 8       /* A8-A11 */
#define PIN_AM8 10    /* P7.00 */
#define PIN_AM9 12    /* P7.01 */
#define PIN_AM10 11   /* P7.02 */
#define PIN_AM11 13   /* P7.03 */
#define PORT_AH 7     /* P7.16-P7.19 */
#define AH_gp 16      /* P7.16-P7.19 */
#define AH_gm 0xF     /* P7.16-P7.19 */
#define AH_vp 12      /* A12-A15 */
#define PIN_AH12 8    /* P7.16 */
#define PIN_AH13 7    /* P7.17 */
#define PIN_AH14 36   /* P7.18 */
#define PIN_AH15 37   /* P7.19 */
#define PORT_CNTL 9   /* GPIO9 */
#define CNTL_gp 4     /* P9.04-P9.07 */
#define CNTL_gm 0xF   /* CNTL0-CNTL3 */
#define CNTL_vp 0     /* CNTL0-CNTL3 */
#define PIN_VP 2      /* P9.04 */
#define PIN_RW 3      /* P9.05 */
#define PIN_SYNC 4    /* P9.06 */
#define PIN_PHI2O 33  /* P9.07 */
#define CNTL_VP 0x1   /* CNTL0 */
#define CNTL_RW 0x2   /* CNTL1 */
#define CNTL_SYNC 0x4 /* CNTL2 */
#define PIN_ML 0      /* P6.03 */
#define PIN_SO 1      /* P6.02 */
#define PIN_PHI0 5    /* P9.08 */
#define PIN_PHI1O 29  /* P9.31 */
#define PIN_IRQ 6     /* P7.10 */
#define PIN_NMI 9     /* P7.11 */
#define PIN_RDY 32    /* P7.12 */
#define PIN_RES 28    /* P8.18 */
#define PIN_BE 31     /* P8.22 */
#define PIN_E 30      /* P8.23 */

// Pin alias for W65C816
#define W65C816_VPA PIN_SYNC
#define W65C816_VDA PIN_PHI2O
#define W65C816_ABORT PIN_PHI1O
#define W65C816_MX PIN_SO
#define CNTL_VPA CNTL_SYNC
#define CNTL_VPD 0x8 /* CNTL3 */

#include "pins.h"
#include "signals_mos6502.h"

namespace debugger {
namespace mos6502 {

enum HardwareType : uint8_t {
    HW_MOS6502 = 0,  // SW_MOS6502
    HW_W65C02S = 3,  // SW_W65C02S
    HW_W65C816 = 4,  // SW_W65C816
};

enum SoftwareType : uint8_t {
    SW_MOS6502 = 0,  // HW_MOS6502
    SW_G65SC02 = 1,  // HW_MOS6502
    SW_R65C02 = 2,   // HW_MOS6502
    SW_W65C02S = 3,  // HW_W65C02S
    SW_W65C816 = 4,  // HW_W65C816
};

struct PinsMos6502 final : Pins {
    void reset() override;
    void idle() override;
    bool step(bool show) override;
    void run() override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;
    void setBreakInst(uint32_t addr) const override;

    void execInst(const uint8_t *inst, uint8_t len);
    void captureWrites(const uint8_t *inst, uint8_t len, uint16_t *addr,
            uint8_t *buf, uint8_t max);

    Signals *cycle(uint8_t data);
    bool native65816() const;
    HardwareType hardwareType() const { return _hardType; }
    SoftwareType softwareType() const { return _softType; }

private:
    HardwareType _hardType;
    SoftwareType _softType;

    void checkHardwareType();
    void checkSoftwareType();
    bool stopInst(uint8_t inst) const;

    friend struct RegsMos6502;
    Signals *rawPrepareCycle();
    Signals *prepareCycle();
    Signals *completeCycle(Signals *signals);
    Signals *cycle();
    void suspend();
    void loop();
    bool rawStep();
    void execute(const uint8_t *inst, uint8_t len, uint16_t *addr, uint8_t *buf,
            uint8_t max);

    void printCycles();
    void disassembleCycles();
};

extern struct PinsMos6502 Pins;

}  // namespace mos6502
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
