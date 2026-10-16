#ifndef __PINS_TMS320C25_H__
#define __PINS_TMS320C25_H__

#define PORT_DATA 6    /* GPIO6 */
#define DATA_gp 16     /* P6.16-P6.31 */
#define DATA_gm 0xFFFF /* P6.16-P6.31 */
#define DATA_vp 0      /* D0-D15 */
#define PIN_D0 19      /* P6.16 */
#define PIN_D1 18      /* P6.17 */
#define PIN_D2 14      /* P6.18 */
#define PIN_D3 15      /* P6.19 */
#define PIN_D4 40      /* P6.20 */
#define PIN_D5 41      /* P6.21 */
#define PIN_D6 17      /* P6.22 */
#define PIN_D7 16      /* P6.23 */
#define PIN_D8 22      /* P6.24 */
#define PIN_D9 23      /* P6.25 */
#define PIN_D10 20     /* P6.26 */
#define PIN_D11 21     /* P6.27 */
#define PIN_D12 38     /* P6.28 */
#define PIN_D13 39     /* P6.29 */
#define PIN_D14 26     /* P6.30 */
#define PIN_D15 27     /* P6.31 */
// A0-A15 share P30-P37 through two 74AHCT157 selected by ASEL: low gives
// A0-A3 and A8-A11, high A4-A7 and A12-A15.
#define PORT_AL 7      /* GPIO7 */
#define AL_gp 0        /* P7.00-P7.03 */
#define AL_gm 0xF      /* P7.00-P7.03 */
#define AL_vp 0        /* A0-A3 */
#define PORT_AM 7      /* GPIO7 */
#define AM_gp 0        /* P7.00-P7.03 */
#define AM_gm 0xF      /* P7.00-P7.03 */
#define AM_vp 4        /* A4-A7 */
#define PIN_AL0 10     /* P7.00 */
#define PIN_AL1 12     /* P7.01 */
#define PIN_AL2 11     /* P7.02 */
#define PIN_AL3 13     /* P7.03 */
#define PORT_AH 7      /* GPIO7 */
#define AH_gp 16       /* P7.16-P7.19 */
#define AH_gm 0xF      /* P7.16-P7.19 */
#define AH_vp 8        /* A8-A11 */
#define PORT_AX 7      /* GPIO7 */
#define AX_gp 16       /* P7.16-P7.19 */
#define AX_gm 0xF      /* P7.16-P7.19 */
#define AX_vp 12       /* A12-A15 */
#define PIN_AH0 8      /* P7.16 */
#define PIN_AH1 7      /* P7.17 */
#define PIN_AH2 36     /* P7.18 */
#define PIN_AH3 37     /* P7.19 */
#define PORT_CNTL 9    /* GPIO9 */
#define CNTL_gp 4      /* P9.04-P9.08 */
#define CNTL_gm 0x1F   /* P9.04-P9.08 */
#define CNTL_vp 0      /* CNTL0-CNTL4 */
#define CNTL_PS 0x01   /* CNTL0 */
#define CNTL_DS 0x02   /* CNTL1 */
#define CNTL_IS 0x04   /* CNTL2 */
#define CNTL_RW 0x08   /* CNTL3 */
#define CNTL_STRB 0x10 /* CNTL4 */
#define PIN_PS 2       /* P9.04 */
#define PIN_DS 3       /* P9.05 */
#define PIN_IS 4       /* P9.06 */
#define PIN_RW 33      /* P9.07 */
#define PIN_STRB 5     /* P9.08 */
#define PIN_XF 0       /* P6.03 */
#define PIN_BIO 1      /* P6.02 */
#define PIN_CLKIN 29   /* P9.31 */
#define PIN_INT0 6     /* P7.10 */
#define PIN_INT1 9     /* P7.11 */
#define PIN_READY 32   /* P7.12 */
#define PIN_RS 28      /* P8.18 */
#define PIN_ASEL 31    /* P8.22 */
#define PIN_CLKOUT1 30 /* P8.23 */

#include "pins_tms320.h"
#include "pwm_clock.h"
#include "signals_tms320c25.h"

namespace debugger {
namespace tms320c25 {

struct PinsTms320C25 final : tms320::PinsTms320 {
    PinsTms320C25();

    void idle() override;
    bool step(bool show) override;
    void run() override;

    void printCycles() override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;
    void setBreakInst(uint32_t) const override {}
#ifdef TUNE_CLOCK
    uint32_t clockInput() const override { return _clockInput; }
    bool setClockInput(uint32_t hz) override;
#endif

    // Injection while halted, for RegsTms320C2X: the CPU always waits in a
    // program read, the cycle at Signals::put().
    uint16_t injectRead(uint16_t inst) override;
    /** The injected sequence's next OUT to port 0. */
    uint16_t captureWrite() override;
    /** The injected sequence's next store at |addr|. */
    uint16_t captureData(uint16_t addr);
    /** Answer the injected sequence's next IN from port 0. */
    void injectIo(uint16_t data);
    /** Answer the injected sequence's next load from |addr|. */
    void injectData(uint16_t addr, uint16_t data);
    /** The CPU's next fetch address. */
    uint16_t fetchAddr() const;
    /** Branch to |pc|, leaving its fetch pending; returns where it went. */
    uint16_t branchTo(uint16_t pc);
    void setBio(bool high) override;

private:
    PwmClock<PIN_CLKIN> _clock;
#ifdef TUNE_CLOCK
    // CLKIN's rate, which K sets.
    uint32_t _clockInput;
#endif
    void resetPins() override;
    void loop();

    // Where the halt landed: the next instruction; the one the NOPs fed
    // last stood in for; and whether an interrupt was taken on the way,
    // pushing a return address among those NOPs.
    struct Halt {
        uint16_t pc;
        uint16_t ret;
        bool interrupt;
    };
    Halt drain(bool table);

    uint16_t transfer(bool read, bool io, uint16_t addr, uint16_t data);
    Signals *prepareCycle();
    void completeCycle(Signals *s);
    Signals *nextCycle(Signals *s);

    void printBacktrace() override;
};

}  // namespace tms320c25
}  // namespace debugger
#endif /* __PINS_TMS320C25_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
