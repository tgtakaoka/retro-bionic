#ifndef __PINS_H8300_H__
#define __PINS_H8300_H__

#define PORT_D 6   /* GPIO6 */
#define D_gp 16    /* P6.16-P6.23 */
#define D_gm 0xFF  /* P6.16-P6.23 */
#define D_vp 0     /* D0-D7 */
#define PIN_D0 19  /* P6.16 */
#define PIN_D1 18  /* P6.17 */
#define PIN_D2 14  /* P6.18 */
#define PIN_D3 15  /* P6.19 */
#define PIN_D4 40  /* P6.20 */
#define PIN_D5 41  /* P6.21 */
#define PIN_D6 17  /* P6.22 */
#define PIN_D7 16  /* P6.23 */
#define PORT_AL 6  /* GPIO6 */
#define AL_gp 24   /* P6.24-P6.31 */
#define AL_gm 0xFF /* P6.24-P6.31 */
#define AL_vp 0    /* A0-A7 */
#define PIN_AL0 22 /* P6.24 */
#define PIN_AL1 23 /* P6.25 */
#define PIN_AL2 20 /* P6.26 */
#define PIN_AL3 21 /* P6.27 */
#define PIN_AL4 38 /* P6.28 */
#define PIN_AL5 39 /* P6.29 */
#define PIN_AL6 26 /* P6.30 */
#define PIN_AL7 27 /* P6.31 */
// A8-A11 on P30-P33 and A12-A15 on P34-P37, one read of GPIO7.
#define PORT_AH 7        /* GPIO7 */
#define AH_gp 0          /* P7.00-P7.03, P7.16-P7.19 */
#define AH_gm 0x000F000F /* P30-P33, P34-P37 */
#define AH_vp 0
#define PIN_AH8 10  /* P7.00 */
#define PIN_AH9 12  /* P7.01 */
#define PIN_AH10 11 /* P7.02 */
#define PIN_AH11 13 /* P7.03 */
#define PIN_AH12 8  /* P7.16 */
#define PIN_AH13 7  /* P7.17 */
#define PIN_AH14 36 /* P7.18 */
#define PIN_AH15 37 /* P7.19 */
// The strobes and the system clock ø, one read of GPIO9.
#define PIN_AS 2     /* P9.04 */
#define PIN_RD 3     /* P9.05 */
#define PIN_WR 4     /* P9.06 */
#define PIN_IOS 33   /* P9.07 */
#define PIN_PHI 5    /* P9.08 */
#define PORT_CNTL 9  /* GPIO9 */
#define CNTL_gp 4    /* P9.04-P9.08 */
#define CNTL_gm 0x1F /* P9.04-P9.08 */
#define CNTL_vp 0
#define PIN_TXD 0    /* P6.03 */
#define PIN_RXD 1    /* P6.02 */
#define PIN_EXTAL 29 /* P9.31 */
#define PIN_IRQ0 6   /* P7.10 */
#define PIN_NMI 9    /* P7.11 */
#define PIN_WAIT 32  /* P7.12 */
#define PIN_RES 28   /* P8.18 */
#define PIN_IRQ1 31  /* P8.22 */
#define PIN_IRQ2 30  /* P8.23 */

#include "pins.h"
#include "pwm_clock.h"
#include "regs_h8300.h"
#include "signals_h8300.h"

namespace debugger {
namespace h8300 {

// The interrupt an emulated device raises: its vector number (H8/325
// Series Hardware Manual Table 4-2).
enum IntrName : uint8_t {
    INTR_NONE = 0,
    INTR_IRQ0 = 4,
    INTR_IRQ1 = 5,
    INTR_IRQ2 = 6,
};

struct PinsH8300 final : Pins {
    using Clock = PwmClock<PIN_EXTAL>;

    PinsH8300();

    // EXTAL runs free and the CPU waits in Tw: nothing to clock.
    void idle() override {}
    bool step(bool show) override;
    void run() override;

    void printCycles() override;
    uint32_t clockInput() const override { return _clockInput; }
    bool setClockInput(uint32_t hz) override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;
    void setBreakInst(uint32_t addr) const override;

    // Where an injected sequence leaves off: back at its own origin, or --
    // given as a plain address -- the target of the jump it ends with.
    static constexpr uint32_t EXIT_ORG = UINT32_MAX;

    // Run |inst| on the parked CPU: its reads inside the window are fed
    // from |inst| by address, the prefetch just past it gets NOPs, other
    // reads come from memory, and every write is captured, never stored.
    // The first |max| bytes written go to |buf|; the count captured is
    // returned. |org| is where the CPU is parked, and is advanced to where
    // the sequence leaves off. Every sequence ends in a taken jump.
    void execInst(const uint8_t *inst, uint_fast8_t len, uint16_t &org,
            uint32_t exit);
    uint_fast8_t captureWrites(const uint8_t *inst, uint_fast8_t len,
            uint8_t *buf, uint_fast8_t max, uint16_t &org, uint32_t exit);
    // Whether the last sequence ended anywhere but its exit.
    bool cutShort() const { return _cutShort; }
    // Complete the vector read the CPU is parked in with |handler|, and
    // park it again at the handler's first fetch.
    bool answerVector(uint16_t vecAddr, uint16_t handler);

private:
    bool _cutShort = false;
    // EXTAL's rate, and the Teensy cycles in one of its clocks.
    uint32_t _clockInput;
    uint32_t _clockCycles;
    Clock _clock;
    void resetPins() override;
    void printBacktrace() override;
    bool setupBus();
    bool loop();
    bool isBreak(const Signals *s, uint16_t &pc) const;
    bool maskBreak(Signals *s, uint16_t pc);
    bool findFrame(const Signals *s, RegsH8300::Frame &frame,
            const Signals *&from) const;

    Signals *prepareCycle();
    // Gives up after |clocks| without #AS: the CPU sleeps.
    Signals *prepareCycle(uint_fast16_t clocks);
    Signals *resumeCycle(uint16_t addr);
    Signals *completeCycle(Signals *s);
    void serveRead(Signals *s);
    void storeWrite(Signals *s);
    uint_fast8_t execute(const uint8_t *inst, uint_fast8_t len, uint8_t *buf,
            uint_fast8_t max, uint16_t &org, uint32_t exit);
    bool suspend(uint32_t org);
};

}  // namespace h8300
}  // namespace debugger
#endif /* __PINS_H8300_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
