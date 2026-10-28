#ifndef __PINS_DCT11_H__
#define __PINS_DCT11_H__

// The DCT11 board (schematics/dct11).
#define PORT_DAL 6    /* GPIO6 */
#define DAL_gp 16     /* P6.16-P6.31 */
#define DAL_gm 0xFFFF /* P6.16-P6.31 */
#define DAL_vp 0      /* DAL0-DAL15 */
#define PORT_DALL 6   /* GPIO6 */
#define DALL_gp 16    /* P6.16-P6.23 */
#define DALL_gm 0xFF  /* P6.16-P6.23 */
#define DALL_vp 0     /* DAL0-DAL7 */
#define PIN_DAL0 19   /* P6.16 */
#define PIN_DAL1 18   /* P6.17 */
#define PIN_DAL2 14   /* P6.18 */
#define PIN_DAL3 15   /* P6.19 */
#define PIN_DAL4 40   /* P6.20 */
#define PIN_DAL5 41   /* P6.21 */
#define PIN_DAL6 17   /* P6.22 */
#define PIN_DAL7 16   /* P6.23 */
#define PIN_DAL8 22   /* P6.24 */
#define PIN_DAL9 23   /* P6.25 */
#define PIN_DAL10 20  /* P6.26 */
#define PIN_DAL11 21  /* P6.27 */
#define PIN_DAL12 38  /* P6.28 */
#define PIN_DAL13 39  /* P6.29 */
#define PIN_DAL14 26  /* P6.30 */
#define PIN_DAL15 27  /* P6.31 */
// SEL0-SEL1, #RAS, #CAS, R/#WLB, R/#WHB, PI and #BCLR all on GPIO7:
// one read takes a transaction's control; the strobes sit on FlexIO3
// pads.
#define PORT_SEL 7    /* GPIO7 */
#define SEL_gp 0      /* P7.00-P7.01 */
#define SEL_gm 0x3    /* P7.00-P7.01 */
#define SEL_vp 0
#define PIN_SEL0 10   /* P7.00 P30 */
#define PIN_SEL1 12   /* P7.01 P31 */
#define PIN_AI0 11    /* P7.02 P32 #DMR */
#define PIN_AI1 13    /* P7.03 P33 #CP3 */
#define PORT_CNTL 7   /* GPIO7 */
#define CNTL_gp 16    /* P7.16-P7.19 */
#define CNTL_gm 0xF   /* P7.16-P7.19 */
#define CNTL_vp 0
#define PIN_RAS 8     /* P7.16 P34 */
#define PIN_CAS 7     /* P7.17 P35 */
#define PIN_RWLB 36   /* P7.18 P36 */
#define PIN_RWHB 37   /* P7.19 P37 */
// #CP2, #CP1, #CP0 and #VEC in one write: an interrupt request's code.
#define PORT_CPV 9    /* GPIO9 */
#define CPV_gp 4      /* P9.04-P9.07 */
#define CPV_gm 0xF    /* P9.04-P9.07 */
#define CPV_vp 0      /* AI2-AI5 */
#define PIN_AI2 2     /* P9.04 P40 #CP2 */
#define PIN_AI3 3     /* P9.05 P41 #CP1 */
#define PIN_AI4 4     /* P9.06 P42 #CP0 */
#define PIN_AI5 33    /* P9.07 P43 #VEC */
#define PIN_AI6 0     /* P6.03 P44 #PF */
#define PIN_AI7 1     /* P6.02 P45 #HALT */
#define PIN_COUT 5    /* P9.08 P46 */
#define PIN_XTL1 29   /* P9.31 P47 */
#define PIN_PI 6      /* P7.10 P50 */
#define PIN_BCLR 32   /* P7.12 P52 */
#define PIN_PUP 28    /* P8.18 P53 */
#define PIN_READY 31  /* P8.22 P54 */

#include "pins_pdp11.h"
#include "signals_dct11.h"

namespace debugger {
namespace dct11 {

struct PinsDct11 final : pdp11::PinsPdp11 {
    PinsDct11();

    void idle() override {}
    bool step(bool show) override;
    void run() override;

    void printCycles() override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;
    void setBreakInst(uint32_t addr) const override;

    void execInst(const uint16_t *inst, uint_fast8_t words, uint16_t &org,
            uint32_t exit = EXIT_END, const uint16_t *stack = nullptr,
            uint16_t stackAt = 0, uint_fast8_t stackWords = 0) override;
    void captureWrites(const uint16_t *inst, uint_fast8_t words,
            uint16_t *buf, uint_fast8_t max, uint16_t &org) override;

private:
    // A device read whose side effects wait for the next transaction:
    // every write is preceded by a read of its address, which must not
    // take a received character.
    uint32_t _devRead = 0;
    bool _devPending = false;
    // The AI lines must not change while PI is high: a device that asks
    // inside a transaction is heard after it.
    bool _intRequest = false;
    bool _inCycle = false;
    void driveInt() const;

    void resetPins() override;
    const SignalsImpl *findBacktraceStart() override;
    void printBacktrace() override;

    Signals *prepareCycle();
    Signals *resumeCycle(uint16_t addr);
    Signals *completeCycle(Signals *s);
    void retrieveData(Signals *s);
    void storeData(Signals *s);
    void settleDevRead(const Signals *s);
    void execute(const uint16_t *inst, uint_fast8_t words, uint16_t *buf,
            uint_fast8_t max, uint16_t &org, uint32_t exit,
            const uint16_t *stack, uint16_t stackAt, uint_fast8_t stackWords);
    bool parkAfterFrame(Signals *s, bool step);
    bool suspend(Signals *s);
    bool rawStep();
    bool loop();
};

}  // namespace dct11
}  // namespace debugger
#endif /* __PINS_DCT11_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
