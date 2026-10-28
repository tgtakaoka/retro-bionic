#ifndef __PINS_H16_H__
#define __PINS_H16_H__

// A1/D0-A16/D15, the multiplexed address and data bus. There is no A0:
// #HDS strobes the even byte on D8-D15, #LDS the odd one on D0-D7.
#define PORT_AD 6    /* GPIO6 */
#define AD_gp 16     /* P6.16-P6.31 */
#define AD_gm 0xFFFF /* P6.16-P6.31 */
#define AD_vp 0      /* A1/D0-A16/D15 */
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
// A17-A23, PF and ST2-ST0, one read of GPIO7.
#define PORT_ALO 7  /* GPIO7 */
#define ALO_gp 0    /* P7.00-P7.03 */
#define ALO_gm 0xF  /* A17-A20 */
#define ALO_vp 0
#define PORT_AHI 7  /* GPIO7 */
#define AHI_gp 16   /* P7.16-P7.18 */
#define AHI_gm 0x7  /* A21-A23 */
#define AHI_vp 4
#define PORT_ST 7   /* GPIO7 */
#define ST_gp 10    /* P7.10-P7.12 */
#define ST_gm 0x7   /* ST0-ST2 */
#define ST_vp 0
#define PORT_PF 7   /* GPIO7 */
#define PF_gp 19    /* P7.19 */
#define PF_gm 0x1
#define PF_vp 3
#define PORT_UPPER 7 /* GPIO7 */
#define PIN_AH17 10  /* P7.00 */
#define PIN_AH18 12  /* P7.01 */
#define PIN_AH19 11  /* P7.02 */
#define PIN_AH20 13  /* P7.03 */
#define PIN_AH21 8   /* P7.16 */
#define PIN_AH22 7   /* P7.17 */
#define PIN_AH23 36  /* P7.18 */
#define PIN_PF 37   /* P7.19 */
#define PIN_ST0 6   /* P7.10 */
#define PIN_ST1 9   /* P7.11 */
#define PIN_ST2 32  /* P7.12 */
// The strobes and the system clock, one read of GPIO9.
#define PIN_AS 2      /* P9.04 */
#define PIN_RW 3      /* P9.05 */
#define PIN_HDS 4     /* P9.06 */
#define PIN_LDS 33    /* P9.07 */
#define PIN_PHI 5     /* P9.08 */
#define PORT_CNTL 9   /* GPIO9 */
#define CNTL_gp 4     /* P9.04-P9.08 */
#define CNTL_gm 0x1F  /* P9.04-P9.08 */
#define CNTL_vp 0
#define PIN_EXTAL 29  /* P9.31 */
#define PIN_IRQ0 0    /* P6.03 */
#define PIN_NMI 1     /* P6.02 */
#define PIN_RES 28    /* P8.18 */
#define PIN_WAIT 31   /* P8.22 */
#define PIN_BRTRY 30  /* P8.23 */

#include "pins.h"
#include "pwm_clock.h"
#include "regs_h16.h"
#include "signals_h16.h"

namespace debugger {
namespace h16 {

struct PinsH16 final : Pins {
    using Clock = PwmClock<PIN_EXTAL>;

    PinsH16();

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
    // from |inst| by address, other program reads get NOPs, and every
    // write is captured, never stored. The first |max| bytes written go
    // to |buf| and their addresses to |addrs|; the count captured is
    // returned. |org| is where the CPU is parked, and is advanced to
    // where the sequence leaves off. Every sequence ends in a taken jump:
    // the prefetch runs up to 8 bytes past the instruction executing.
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
    // EXTAL's rate, and the Teensy cycles in one of its clocks.
    uint32_t _clockInput;
    uint32_t _clockCycles;
    Clock _clock;
    void resetPins() override;
    void printBacktrace() override;
    void setupBus();
    bool loop();
    uint32_t isException(const Signals *s, RegsH16::Frame &frame,
            const Signals *&from) const;
    bool isBreak(const Signals *s, RegsH16::Frame &frame,
            const Signals *&from) const;
    bool isHalt(const Signals *s, RegsH16::Frame &frame,
            const Signals *&from) const;
    bool maskBreak(Signals *s, const RegsH16::Frame &frame);
    bool isVector(const Signals *s, uint32_t offset) const;

    Signals *prepareCycle();
    // Gives up after |clocks| without #AS: the CPU sleeps or halted.
    Signals *prepareCycle(uint_fast16_t clocks);
    Signals *resumeCycle(uint32_t addr);
    Signals *completeCycle(Signals *s);
    void serveRead(Signals *s);
    void storeWrite(Signals *s);
    uint_fast8_t execute(const uint8_t *inst, uint_fast8_t len, uint8_t *buf,
            uint32_t *addrs, uint_fast8_t max, uint32_t &org, uint32_t exit);
    bool suspend(uint32_t org);
};

}  // namespace h16
}  // namespace debugger
#endif /* __PINS_H16_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
