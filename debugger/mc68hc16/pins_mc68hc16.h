#ifndef __PINS_MC68HC16_H__
#define __PINS_MC68HC16_H__

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
// A0-A23, FC2:FC0 and SIZ1:SIZ0 share P30-P37 through four 74HCS153
// selected by ASEL1:ASEL0; AL carries A0-A3 (00), A4-A7 (01), A12-A15
// (10) and A8-A11 (11), AH A16-A19, A20-A23, SIZ1:SIZ0 and FC2:FC0. One
// read of GPIO7 takes both nibbles.
#define PORT_ADDR 7        /* GPIO7 */
#define ADDR_gp 0          /* P7.00-P7.03, P7.16-P7.19 */
#define ADDR_gm 0x000F000F /* AL, AH */
#define ADDR_vp 0
#define PIN_AL0 10 /* P7.00 */
#define PIN_AL1 12 /* P7.01 */
#define PIN_AL2 11 /* P7.02 */
#define PIN_AL3 13 /* P7.03 */
#define PIN_AH0 8  /* P7.16 */
#define PIN_AH1 7  /* P7.17 */
#define PIN_AH2 36 /* P7.18 */
#define PIN_AH3 37 /* P7.19 */
// The strobes and the pipeline state, one read of GPIO9.
#define PIN_AS 2      /* P9.04 */
#define PIN_RW 3      /* P9.05 */
#define PIN_IPIPE0 4  /* P9.06 */
#define PIN_IPIPE1 33 /* P9.07 */
#define PIN_DS 5      /* P9.08 */
#define PORT_CNTL 9   /* GPIO9 */
#define CNTL_gp 4     /* P9.04-P9.08 */
#define CNTL_gm 0x1F  /* P9.04-P9.08 */
#define CNTL_vp 0
#define PIN_DSACK0 0  /* P6.03 */
#define PIN_DSACK1 1  /* P6.02 */
#define PIN_EXTAL 29  /* P9.31 */
#define PIN_IRQ7 6    /* P7.10 */
#define PIN_IRQ1 9    /* P7.11 */
#define PIN_CLKOUT 32 /* P7.12 */
#define PIN_RESET 28  /* P8.18 */
#define PIN_ASEL0 31  /* P8.22 */
#define PIN_ASEL1 30  /* P8.23 */

#include "pins.h"
#include "signals_mc68hc16.h"

namespace debugger {
namespace mc68hc16 {

struct RegsMc68hc16;

struct PinsMc68hc16 final : Pins {
    PinsMc68hc16();

    void idle() override;
    bool step(bool show) override;
    void run() override;

    void printCycles() override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;
    void setBreakInst(uint32_t addr) const override;

    // Where an injected sequence leaves off: back at its own origin, or --
    // given as a plain address -- the target of the jump it ends with.
    static constexpr uint32_t EXIT_ORG = UINT32_MAX;

    // Run |inst| on the parked CPU: its reads inside the window are fed
    // from |inst| by address, other program reads get NOPs, and every
    // write is captured, never stored. The first |max| bytes written go
    // to |buf| and their addresses to |addrs|; the count captured is
    // returned. |org| is where the CPU is parked, and is advanced to
    // where the sequence leaves off. Instructions are words, so |org| and
    // |len| are even, and every sequence ends in a taken branch: the
    // prefetch reaches the exit before the last instructions execute.
    void execInst(const uint8_t *inst, uint_fast8_t len, uint32_t &org,
            uint32_t exit);
    uint_fast8_t captureWrites(const uint8_t *inst, uint_fast8_t len,
            uint8_t *buf, uint32_t *addrs, uint_fast8_t max, uint32_t &org,
            uint32_t exit);
    // Whether the last sequence ended anywhere but its exit.
    bool cutShort() const { return _cutShort; }
    // Complete the vector read the CPU is parked in with |handler|, and
    // park it again at the handler's first fetch.
    bool answerVector(uint32_t vecAddr, uint32_t handler);

private:
    bool _cutShort = false;
    void resetPins() override;
    const SignalsImpl *findBacktraceStart() override;
    void printBacktrace() override;
    void setupBus();
    bool loop();
    bool isSwiBreak(Signals *s, const Signals *&from);
    void markStarts();

    Signals *prepareCycle();
    Signals *resumeCycle(uint32_t addr);
    Signals *completeCycle(Signals *s);
    uint_fast8_t execute(const uint8_t *inst, uint_fast8_t len, uint8_t *buf,
            uint32_t *addrs, uint_fast8_t max, uint32_t &org, uint32_t exit);
    bool suspend(uint32_t org, uint_fast8_t holdOff = 0);
};

}  // namespace mc68hc16
}  // namespace debugger
#endif /* __PINS_MC68HC16_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
