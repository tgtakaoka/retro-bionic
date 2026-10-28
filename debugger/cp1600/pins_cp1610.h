#ifndef __PINS_CP1610_H__
#define __PINS_CP1610_H__

#define PORT_DATA 6     /* GPIO6 */
#define DATA_gp 16      /* P6.16-P6.31 */
#define DATA_gm 0xFFFF  /* P6.16-P6.31 */
#define DATA_vp 0       /* D0-D15 */
#define PIN_D0 19       /* P6.16 */
#define PIN_D1 18       /* P6.17 */
#define PIN_D2 14       /* P6.18 */
#define PIN_D3 15       /* P6.19 */
#define PIN_D4 40       /* P6.20 */
#define PIN_D5 41       /* P6.21 */
#define PIN_D6 17       /* P6.22 */
#define PIN_D7 16       /* P6.23 */
#define PIN_D8 22       /* P6.24 */
#define PIN_D9 23       /* P6.25 */
#define PIN_D10 20      /* P6.26 */
#define PIN_D11 21      /* P6.27 */
#define PIN_D12 38      /* P6.28 */
#define PIN_D13 39      /* P6.29 */
#define PIN_D14 26      /* P6.30 */
#define PIN_D15 27      /* P6.31 */
#define PIN_EBCA0 10    /* P7.00 */
#define PIN_EBCA1 12    /* P7.01 */
#define PIN_EBCA2 11    /* P7.02 */
#define PIN_EBCA3 13    /* P7.03 */
#define PIN_ENSWI 8     /* P7.16 */
#define PIN_BUSREQ 36   /* P7.18 */
#define PIN_BUSACK 37   /* P7.19 */
#define PORT_CNTL 9     /* GPIO9 */
#define CNTL_gp 4       /* P9.04-P9.07 */
#define CNTL_gm 0xF     /* P9.04-P9.07 */
#define CNTL_vp 0       /* BDIR, BC1, BC2, #PCIT */
#define PIN_BDIR 2      /* P9.04 */
#define PIN_BC1 3       /* P9.05 */
#define PIN_BC2 4       /* P9.06 */
#define PIN_PCIT 33     /* P9.07 */
#define CNTL_FETCH 0x10 /* not a pin */
#define PIN_TCI 0       /* P6.03 */
#define PIN_EBCI 1      /* P6.02 */
#define PIN_PHI1 5      /* P9.08 */
#define PIN_PHI2 29     /* P9.31 */
#define PIN_INTR 6      /* P7.10 */
#define PIN_INTRM 9     /* P7.11 */
#define PIN_STPST 32    /* P7.12 */
#define PIN_MSYNC 28    /* P8.18 */
#define PIN_BDRDY 31    /* P8.22 */
#define PIN_HALT 30     /* P8.23 */

#include "pins.h"
#include "pwm_clock.h"
#include "signals_cp1610.h"

namespace debugger {
namespace cp1600 {

struct Phases;

struct PinsCp1610 final : Pins {
    PinsCp1610();

    void resetPins() override;
    void idle() override;
    bool step(bool show) override;
    void run() override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;
    void setBreakInst(uint32_t addr) const override;
    void printCycles() override;
#ifdef TUNE_CLOCK
    uint32_t clockInput() const override;
    bool setClockInput(uint32_t hz) override;
#endif

    /**
     * Start the halted CPU, answer its reads with |inst| in order and
     * capture its writes into |buf|, then halt it again. Returns the
     * address of the first injected instruction, the CPU's PC.
     */
    uint16_t injectInsts(const uint16_t *inst, uint_fast8_t len,
            uint16_t *buf = nullptr, uint_fast8_t max = 0);
    // Returns whether INTFF is set: probe it with #INTRM.
    bool probeIntff();
    // Start the halted CPU and answer its reads with |inst|, leaving the
    // CPU at the transaction following them.
    void resumeInsts(const uint16_t *inst, uint_fast8_t len);

private:
    PwmClock<PIN_PHI1> _phi1;
    PwmClock<PIN_PHI2> _phi2;
#ifdef TUNE_CLOCK
    // The clock period in bus clock counts, which K sets.
    uint16_t _counts;
#endif
    // The bus timing in Teensy cycles, from the free-running PHI1/PHI2.
    uint32_t _microcycle;  // TS1 to TS1
    uint32_t _addrAt;      // TS1 to reading D0-D15 in TS4
    uint32_t _releaseAt;   // TS1 to releasing read data, past TS4
    uint32_t _next;        // TS1 of the next microcycle to serve
    uint32_t _ts1;         // TS1 of the microcycle being served
    bool _holding;         // BDRDY holds the CPU after a BAR
    // Measurement only, for the misses report: kept while the bus timing
    // is tuned, to be removed as counting costs the bus loop time too.
    uint32_t _missed;      // microcycles that passed unserved
    uint32_t _late;        // reads whose data came after TS3 began
    uint32_t _held;        // when BDRDY began to hold the CPU
    uint32_t _overheld;    // holds longer than the CPU allows
    uint16_t _addr;   // emulated external address register
    uint16_t _prefetch;  // memory at _addr, read ahead of its DTB
    bool _prefetched;    // _prefetch holds memory at _addr
    bool _breakAhead;    // a break point is at _addr
    uint16_t _vector;    // the devices' vector, read at INTAK for the IAB
    uint8_t _remain;  // bus transactions until the next fetch
    bool _sdbd;       // the last instruction was SDBD
    bool _writes;     // the instruction under way is an MVO
    bool _branch;     // the last instruction was a branch, whose
    uint16_t _disp;   // displacement is at _disp unless it is not taken
    bool _intrm;      // #INTRM requested by devices

    const Phases &phases() const;
    void setPhases() const;
    void setTiming();
    void regrid();
    Signals *nextCode();
    void reportMisses(const char *where);
    Signals *prepareCycle(
            uint_fast16_t limit = 64, bool hold = false, bool busak = false);
    void release();
    void latchBar();
    void prefetch();
    Signals *completeCycle(Signals *s);
    Signals *resumeCycle();
    void trackFetch(Signals *s);
    bool fetching(const Signals *s) const;
    void unhalt();
    void startInsts();
    static uint16_t parking(uint_fast8_t i);
    bool loop();
    void halted();
    bool rawStep();

    const SignalsImpl *findBacktraceStart() override;
    void printBacktrace() override;
};

}  // namespace cp1600
}  // namespace debugger
#endif /* __PINS_CP1610_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
