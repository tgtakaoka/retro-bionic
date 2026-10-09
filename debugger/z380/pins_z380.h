#ifndef __PINS_Z380_H__
#define __PINS_Z380_H__

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
// A0-A31 share P30-P37 through four 74HCS153 selected by ASEL1:ASEL0;
// AL carries A0-A3 (00), A4-A7 (01), A12-A15 (10), A8-A11 (11), and AH
// the same nibbles of A16-A31. One read of GPIO7 takes both nibbles.
#define PORT_ADDR 7        /* GPIO7 */
#define ADDR_gp 0          /* P7.00-P7.03, P7.16-P7.19 */
#define ADDR_gm 0x000F000F /* AL, AH */
#define ADDR_vp 0          /* nibbles of A0-A15 and A16-A31 */
#define PIN_AL0 10         /* P7.00 */
#define PIN_AL1 12         /* P7.01 */
#define PIN_AL2 11         /* P7.02 */
#define PIN_AL3 13         /* P7.03 */
#define PIN_AH0 8          /* P7.16 */
#define PIN_AH1 7          /* P7.17 */
#define PIN_AH2 36         /* P7.18 */
#define PIN_AH3 37         /* P7.19 */
#define PIN_M1 2           /* P9.04 */
#define PIN_MRD 3          /* P9.05 */
#define PIN_MWR 4          /* P9.06 */
#define PIN_IORD 33        /* P9.07 */
#define PIN_IOWR 5         /* P9.08 */
#define PORT_CNTL 9        /* GPIO9 */
#define CNTL_gp 4          /* P9.04-P9.08 */
#define CNTL_gm 0x1F       /* P9.04-P9.08 */
#define CNTL_vp 0
#define PIN_ASEL0 0  /* P6.03 */
#define PIN_ASEL1 1  /* P6.02 */
#define PIN_CLKI 29  /* P9.31 */
#define PIN_INT0 6   /* P7.10 */
#define PIN_NMI 9    /* P7.11 */
#define PIN_WAIT 32  /* P7.12 */
#define PIN_RESET 28 /* P8.18 */
#define PIN_BLEN 31  /* P8.22 */
#define PIN_BHEN 30  /* P8.23 */
/* #BLEN and #BHEN together, one read of GPIO8 */
#define PORT_BEN 8
#define BEN_gp 22
#define BEN_gm 0x3
#define BEN_vp 0

#include "pins.h"
#include "signals_z380.h"

namespace debugger {
namespace z380 {

struct PinsZ380 final : Pins {
    PinsZ380();

    void idle() override;
    bool step(bool show) override;
    void run() override;

    void printCycles() override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;
    void setBreakInst(uint32_t addr) const override;

    // Where an injected sequence leaves off: past its last byte, back
    // at its own origin, or -- given as a plain address -- the target
    // of a jump it ends with.
    static constexpr uint32_t EXIT_END = UINT32_MAX;
    static constexpr uint32_t EXIT_ORG = UINT32_MAX - 1;

    // Run |inst| on the parked CPU: its memory reads are fed from |inst|
    // by address and its writes are captured into |buf|, never touching
    // real memory. Returns the lowest address written. |org| is where the
    // CPU is parked, and is advanced to where the sequence leaves off, so
    // a caller chaining sequences passes it on. Fetches are words, so
    // |org| is even and a sequence that runs off its end has an even
    // length.
    void execInst(const uint8_t *inst, uint_fast8_t len, uint32_t &org,
            uint32_t exit = EXIT_END);
    uint32_t captureWrites(const uint8_t *inst, uint_fast8_t len, uint8_t *buf,
            uint_fast8_t max, uint32_t &org, uint32_t exit = EXIT_END);
    // Where the first write of the last captureWrites() went.
    uint32_t firstWrite() const { return _firstWrite; }

private:
    uint32_t _firstWrite = 0;
    bool _cutShort = false;  // the last execute() ended before its end
#ifdef PROFILE_CYCLES
    void dataLoopback();
#endif
    void resetPins() override;
    const SignalsImpl *findBacktraceStart() override;
    void printBacktrace() override;
    void setupBus();
    bool isRst38Break(Signals *s, Signals *&from);
    struct CodeMemory;
    bool walkRing(const CodeMemory &memory);
    bool loop();

    Signals *prepareCycle();
    Signals *resumeCycle(uint32_t addr);
    Signals *completeCycle(Signals *s);
    static uint16_t zbusWord(
            const uint8_t *inst, uint_fast8_t len, uint32_t org, uint32_t addr);
    uint32_t execute(const uint8_t *inst, uint_fast8_t len, uint8_t *buf,
            uint_fast8_t max, uint32_t &org, uint32_t exit);
    bool suspend(uint32_t org, uint_fast8_t holdOff = 0);
};

}  // namespace z380
}  // namespace debugger
#endif /* __PINS_Z380_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
