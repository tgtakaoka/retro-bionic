#ifndef __PINS_NS32016_H__
#define __PINS_NS32016_H__

// The BionicNS32016 board; the BionicNS32081 board adds the FPU on the
// CPU's slave bus, which the Teensy does not see.
#define PORT_AD 6    /* GPIO6 */
#define AD_gp 16     /* P6.16-P6.31 */
#define AD_gm 0xFFFF /* P6.16-P6.31 */
#define AD_vp 0      /* AD0-AD15 */
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
// A16-A23 share P30-P33 through a 74AHCT157: ASEL low A16-A19, high
// A20-A23. ST0-ST3 are on the same port, so one read takes both.
#define PORT_AH 7  /* GPIO7 */
#define AH_gp 0    /* P7.00-P7.03 */
#define AH_gm 0xF  /* P7.00-P7.03 */
#define AH_vp 0    /* A16-A19 or A20-A23 */
#define PIN_AH0 10 /* P7.00 P30 */
#define PIN_AH1 12 /* P7.01 P31 */
#define PIN_AH2 11 /* P7.02 P32 */
#define PIN_AH3 13 /* P7.03 P33 */
#define PORT_ST 7  /* GPIO7 */
#define ST_gp 16   /* P7.16-P7.19 */
#define ST_gm 0xF  /* P7.16-P7.19 */
#define ST_vp 0    /* ST0-ST3 */
#define PIN_ST0 8  /* P7.16 P34 */
#define PIN_ST1 7  /* P7.17 P35 */
#define PIN_ST2 36 /* P7.18 P36 */
#define PIN_ST3 37 /* P7.19 P37 */
// The strobes in one read of GPIO9.
#define PORT_CNTL 9   /* GPIO9 */
#define CNTL_gp 4     /* P9.04-P9.07 */
#define CNTL_gm 0xF   /* P9.04-P9.07 */
#define CNTL_vp 0     /* #ADS #DS #DDIN #HBE */
#define PIN_ADS 2     /* P9.04 P40 */
#define PIN_DS 3      /* P9.05 P41 */
#define PIN_DDIN 4    /* P9.06 P42 */
#define PIN_HBE 33    /* P9.07 P43 */
#define CNTL_ADS 0x1  /* in CNTL */
#define CNTL_DS 0x2   /* in CNTL */
#define CNTL_DDIN 0x4 /* in CNTL */
#define CNTL_HBE 0x8  /* in CNTL */
// #PFS and U/#S ride GPIO6 with AD0-AD15.
#define PIN_PFS 0     /* P6.03 P44 */
#define PIN_US 1      /* P6.02 P45 */
#define US_bm (1 << 2) /* in GPIO6 */
#define PIN_CLK 5     /* P9.08 P46, FlexPWM2_1_A */
#define PIN_ASEL 29   /* P9.31 P47 */
#define PIN_INT 6     /* P7.10 P50 */
#define PIN_NMI 9     /* P7.11 P51 */
#define PIN_RDY 32    /* P7.12 P52 */
#define PIN_RST 28    /* P8.18 P53 */
#define PIN_HLDA 31   /* P8.22 P54 */
#define PIN_HOLD 30   /* P8.23 P55 */

#include "pins.h"
#include "pwm_clock.h"
#include "signals_ns32016.h"

namespace debugger {
namespace ns32016 {

struct RegsNs32016;

struct PinsNs32016 final : Pins {
    PinsNs32016(bool fpu);
    // The NS32081 board: CFG.F set, F0-F7 and FSR saved.
    bool fpu() const { return _fpu; }

    // The clock runs by itself: the CPU is dynamic and must never stop.
    void idle() override {}
    bool step(bool show) override;
    void run() override;

    void printCycles() override;
    uint32_t clockInput() const override { return _clockInput; }
    bool setClockInput(uint32_t hz) override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;
    void setBreakInst(uint32_t addr) const override;

    // A read the debugger answers by address while a sequence runs.
    struct Keyed {
        uint32_t addr;
        uint16_t data;
    };
    // Run |inst| on the CPU parked at |org|: its fetches inside the window
    // are fed from |inst| by address, other fetches get NOPs, reads of a
    // |keys| address get that word and other reads 0, and every write is
    // captured, never stored. The first |max| bytes written go to |buf|.
    // The sequence ends in a taken branch; the CPU parks on the
    // non-sequential fetch of |exit|, which |org| becomes.
    void execInst(const uint8_t *inst, uint_fast8_t len, uint32_t &org,
            uint32_t exit, const Keyed *keys = nullptr,
            uint_fast8_t nkeys = 0);
    uint_fast8_t captureWrites(const uint8_t *inst, uint_fast8_t len,
            uint8_t *buf, uint_fast8_t max, uint32_t &org, uint32_t exit);

private:
    const bool _fpu;
    PwmClock<PIN_CLK> _clock;
    uint32_t _clockInput;
    // The cycle the CPU is held in, RDY low.
    Signals _parked;

    void resetPins() override;
    const SignalsImpl *findBacktraceStart() override;
    void printBacktrace() override;

    RegsNs32016 *regs() const;
    void park(Signals *s);
    Signals *resumeCycle();
    Signals *prepareCycle(uint32_t timeoutCycles);
    Signals *prepareCycle();
    Signals *completeCycle(Signals *s);
    void serveRead(Signals *s);
    void storeWrite(Signals *s);
    uint_fast8_t execute(const uint8_t *inst, uint_fast8_t len, uint8_t *buf,
            uint_fast8_t max, uint32_t &org, uint32_t exit, const Keyed *keys,
            uint_fast8_t nkeys);
    // A trap or interrupt entry as the bus shows it.
    struct Entry {
        const Signals *push;  // the PSR
        const Signals *ack;   // an interrupt's acknowledge, or null
        uint16_t mod;
        uint16_t offset;
    };
    struct Frame {
        uint32_t pc;
        uint16_t mod;
        uint16_t psr;
    };
    // An NMI asserted by a halt and not yet acknowledged.
    bool _nmiPending = false;
    // The program's handler for the trap parked in.
    uint32_t _handler = 0;
    bool trapEntry(const Signals *s, Entry &entry) const;
    bool takeTrap(Signals *s, const Entry &entry, Frame &frame);
    void forwardTrap();
    bool nmiAck(const Signals *ack) const;
    bool suspend(Signals *s);
    bool loop();
};

}  // namespace ns32016
}  // namespace debugger
#endif /* __PINS_NS32016_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
