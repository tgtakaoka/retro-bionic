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
#include "signals_cp1610.h"

namespace debugger {
namespace cp1600 {

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
    uint16_t _addr;   // emulated external address register
    uint8_t _remain;  // bus transactions until the next fetch
    bool _sdbd;       // the last instruction was SDBD
    bool _intrm;      // #INTRM requested by devices
    Signals _held;    // transaction the CPU is left at by resumeInsts()

    Signals *prepareCycle(uint_fast16_t limit = 64);
    Signals *completeCycle(Signals *s);
    Signals *resumeCycle();
    void trackFetch(Signals *s);
    bool fetching(const Signals *s) const;
    void unhalt();
    void startInsts();
    static uint16_t parking(uint_fast8_t i);
    bool loop();
    bool suspend(Signals *intak, int_fast8_t offset);
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
