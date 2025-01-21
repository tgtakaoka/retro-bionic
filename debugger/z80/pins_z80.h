#ifndef __PINS_Z80_H__
#define __PINS_Z80_H__

#define PORT_D 6       /* GPIO6 */
#define D_gp 16        /* P6.16-P6.23 */
#define D_gm 0xFF      /* P6.16-P6.23 */
#define D_vp 0         /* AD0-AD7 */
#define PIN_D0 19      /* P6.16 */
#define PIN_D1 18      /* P6.17 */
#define PIN_D2 14      /* P6.18 */
#define PIN_D3 15      /* P6.19 */
#define PIN_D4 40      /* P6.20 */
#define PIN_D5 41      /* P6.21 */
#define PIN_D6 17      /* P6.22 */
#define PIN_D7 16      /* P6.23 */
#define PORT_AL 6      /* GPIO6 */
#define AL_gp 24       /* P6.24-P6.31 */
#define AL_gm 0xFF     /* P6.24-P6.31 */
#define AL_vp 0        /* A0-A7 */
#define PIN_AL0 22     /* P6.24 */
#define PIN_AL1 23     /* P6.25 */
#define PIN_AL2 20     /* P6.26 */
#define PIN_AL3 21     /* P6.27 */
#define PIN_AL4 38     /* P6.28 */
#define PIN_AL5 39     /* P6.29 */
#define PIN_AL6 26     /* P6.30 */
#define PIN_AL7 27     /* P6.31 */
#define PORT_AM 7      /* GPIO7 */
#define AM_gp 0        /* P7.00-P7.03 */
#define AM_gm 0xF      /* P7.12-P7.15 */
#define AM_vp 8        /* A8-A11 */
#define PIN_AM8 10     /* P7.00 */
#define PIN_AM9 12     /* P7.01 */
#define PIN_AM10 11    /* P7.02 */
#define PIN_AM11 13    /* P7.03 */
#define PORT_AH 7      /* P7.16-P7.19 */
#define AH_gp 16       /* P7.16-P7.19 */
#define AH_gm 0xF      /* P7.16-P7.19 */
#define AH_vp 12       /* A12-A15 */
#define PIN_AH12 8     /* P7.16 */
#define PIN_AH13 7     /* P7.17 */
#define PIN_AH14 36    /* P7.18 */
#define PIN_AH15 37    /* P7.19 */
#define PORT_CNTL 9    /* GPIO9 */
#define CNTL_gp 4      /* P9.04-P9.08 */
#define CNTL_gm 0x1F   /* P9.04-P9.08 */
#define CNTL_vp 0      /* CNTL0-CNTL4 */
#define PIN_RD 2       /* P9.04 */
#define PIN_WR 3       /* P9.05 */
#define PIN_M1 4       /* P9.06 */
#define PIN_MREQ 33    /* P9.07 */
#define PIN_IORQ 5     /* P9.08 */
#define CNTL_RD 0x01   /* CNTL0 */
#define CNTL_WR 0x02   /* CNTL1 */
#define CNTL_M1 0x04   /* CNTL2 */
#define CNTL_MREQ 0x08 /* CNTL3 */
#define CNTL_IORQ 0x10 /* CNTL4 */
#define PIN_HALT 0     /* P6.03 */
#define PIN_RFSH 1     /* P6.02 */
#define PIN_CLK 29     /* P9.31 */
#define PIN_INT 6      /* P7.10 */
#define PIN_NMI 9      /* P7.11 */
#define PIN_WAIT 32    /* P7.12 */
#define PIN_RESET 28   /* P8.18 */
#define PIN_BUSREQ 31  /* P8.22 */
#define PIN_BUSACK 30  /* P8.23 */

#include "pins_z80_base.h"
#include "signals_z80.h"

namespace debugger {
namespace z80 {

struct PinsZ80 final : PinsZ80Base {
    PinsZ80() : PinsZ80Base() {}

    void resetPins() override;
    void idle() override;
    bool step(bool show) override;
    void run() override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;
    void printCycles() override;

private:
    Signals *prepareCycle() const;
    Signals *completeCycle(Signals *signals) const;
    Signals *prepareWait() const;
    Signals *resumeCycle(uint16_t pc) const;

    Signals *inject(uint8_t data) const;
    void loop();
    void suspend();
    bool rawStep();
    void execute(const uint8_t *inst, uint8_t len, uint16_t *addr, uint8_t *buf,
            uint8_t max) override;

    void disassembleCycles();
};

}  // namespace z80
}  // namespace debugger
#endif /* __PINS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
