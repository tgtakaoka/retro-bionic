#ifndef __PINS_MC68008_H__
#define __PINS_MC68008_H__

// D0-D7 on P10-P17.
#define PORT_D 6    /* GPIO6 */
#define D_gp 16     /* P6.16-P6.23 */
#define D_gm 0xFF   /* P6.16-P6.23 */
#define D_vp 0      /* D0-D7 */
#define PIN_D0 19   /* P6.16 */
#define PIN_D1 18   /* P6.17 */
#define PIN_D2 14   /* P6.18 */
#define PIN_D3 15   /* P6.19 */
#define PIN_D4 40   /* P6.20 */
#define PIN_D5 41   /* P6.21 */
#define PIN_D6 17   /* P6.22 */
#define PIN_D7 16   /* P6.23 */
// A0-A7 direct on P20-P27.
#define PORT_AL 6   /* GPIO6 */
#define AL_gp 24    /* P6.24-P6.31 */
#define AL_gm 0xFF  /* P6.24-P6.31 */
#define AL_vp 0     /* A0-A7 */
#define PIN_AL0 22   /* P6.24 */
#define PIN_AL1 23   /* P6.25 */
#define PIN_AL2 20   /* P6.26 */
#define PIN_AL3 21   /* P6.27 */
#define PIN_AL4 38   /* P6.28 */
#define PIN_AL5 39   /* P6.29 */
#define PIN_AL6 26   /* P6.30 */
#define PIN_AL7 27   /* P6.31 */
// A8-A15 share P30-P33 through a 74AHCT157 selected by ASEL (P54): 0
// A8-A11, 1 A12-A15. P34-P37 carry A16-A19 direct on the P board; on the
// FN board a second 157 gives 0 A16-A19, 1 A20-A21. FC2:FC0 sit on
// P50-P52 in the same GPIO7, so one read takes them all.
#define PORT_ADDR 7 /* GPIO7 */
#define PIN_AM0 10  /* P7.00 */
#define PIN_AM1 12  /* P7.01 */
#define PIN_AM2 11  /* P7.02 */
#define PIN_AM3 13  /* P7.03 */
#define PIN_AH0 8   /* P7.16 */
#define PIN_AH1 7   /* P7.17 */
#define PIN_AH2 36  /* P7.18 */
#define PIN_AH3 37  /* P7.19 */
#define PIN_FC0 6   /* P7.10 */
#define PIN_FC1 9   /* P7.11 */
#define PIN_FC2 32  /* P7.12 */
// The strobes and #HALT, one read of GPIO9. P42 is an output here.
#define PORT_CNTL 9  /* GPIO9 */
#define CNTL_gp 4    /* P9.04-P9.08 */
#define CNTL_gm 0x1F /* P9.04-P9.08 */
#define CNTL_vp 0
#define PIN_AS 2     /* P9.04 */
#define PIN_RW 3     /* P9.05 */
#define PIN_IPL20 4  /* P9.06; #IPL2 and #IPL0 tied */
#define PIN_DS 33    /* P9.07 */
#define PIN_HALT 5   /* P9.08 */
#define PIN_DTACK 0  /* P6.03 */
#define PIN_BERR 1   /* P6.02 */
#define PIN_CLK 29   /* P9.31 */
#define PIN_RESET 28 /* P8.18 */
#define PIN_ASEL 31  /* P8.22 */
#define PIN_IPL1 30  /* P8.23 */

#include "pins_mc68000.h"
#include "pwm_clock.h"
#include "regs_mc68000.h"
#include "signals_mc68000.h"

namespace debugger {
namespace mc68000 {

// The MC68008P (DIP48, 20 address lines) and MC68008FN (PLCC52, 22) boards,
// told apart at reset by whether P34-P37 follow ASEL.
struct PinsMc68008 final : PinsMc68000 {
    PinsMc68008();

    // The clock runs free and the parked CPU waits on #DTACK.
    void idle() override {}
    bool step(bool show) override;
    void run() override;

    void printCycles() override;
    uint32_t clockInput() const override { return _clockInput; }
    bool setClockInput(uint32_t hz) override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;
    void setBreakInst(uint32_t addr) const override;

    void execInst(const uint8_t *inst, uint_fast8_t len, uint32_t &org,
            uint32_t exit) override;
    uint_fast8_t captureWrites(const uint8_t *inst, uint_fast8_t len,
            uint8_t *buf, uint32_t *addrs, uint_fast8_t max, uint32_t &org,
            uint32_t exit) override;
    bool answerVector(uint32_t vecAddr, uint32_t handler) override;

private:
    PwmClock<PIN_CLK> _clock;
    // CLK's rate, and the Teensy cycles in one of its clocks.
    uint32_t _clockInput;
    uint32_t _clockCycles;
    // The FN board's 22 address lines, rather than the P board's 20.
    bool _fn = false;
    // A device's interrupt is pending; level 7 is being raised for a halt.
    bool _intr = false;
    bool _halting = false;

    void resetPins() override;
    void printBacktrace() override;
    void detectBoard();
    void setIpl();
    uint32_t wrap(uint32_t addr) const;

    Signals *prepareCycle();
    // Gives up after |clocks| without #AS: the CPU is stopped.
    Signals *prepareCycle(uint_fast16_t clocks);
    // The cycle the CPU is parked in, read again.
    Signals *resumeCycle();
    Signals *completeCycle(Signals *s);
    void getAddr(Signals *s) const;
    void serveRead(Signals *s);
    void storeWrite(Signals *s);
    uint_fast8_t execute(const uint8_t *inst, uint_fast8_t len, uint8_t *buf,
            uint32_t *addrs, uint_fast8_t max, uint32_t &org, uint32_t exit);
    bool findFrame(const Signals *s, RegsMc68000::Frame &frame,
            const Signals *&from) const;
    bool loop();
    bool suspend(Signals *s);
};

}  // namespace mc68000
}  // namespace debugger
#endif /* __PINS_MC68008_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
