#ifndef __PINS_Z8000_H__
#define __PINS_Z8000_H__

// The Z8002 board, and the Z8001 board, which has the same pins plus SN0-SN6,
// ASEL and #SEGT; those are not connected on the Z8002 board.
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
#define PORT_ST 7    /* GPIO7 */
#define ST_gp 0      /* P7.00-P7.03 */
#define ST_gm 0xF    /* P7.00-P7.03 */
#define ST_vp 0      /* ST0-ST3 */
#define PIN_ST0 10   /* P7.00 */
#define PIN_ST1 12   /* P7.01 */
#define PIN_ST2 11   /* P7.02 */
#define PIN_ST3 13   /* P7.03 */
// Z8001: SN0-SN3 share P30-P33 with ST0-ST3 through a 74AHCT157, ASEL
// low picking ST, high SN; SN4-SN6 have their own pins. One read of
// GPIO7 takes all seven.
#define PORT_SN 7        /* GPIO7 */
#define SN_gp 0          /* P7.00-P7.03, P7.16-P7.18 */
#define SN_gm 0x0007000F /* SN0-SN3, SN4-SN6 */
#define SN_vp 0
#define PIN_SN4 8  /* P7.16 P34 */
#define PIN_SN5 7  /* P7.17 P35 */
#define PIN_SN6 36 /* P7.18 P36 */
#define PIN_ASEL 0 /* P6.03 P44 */
#define PIN_SEGT 1 /* P6.02 P45 */
// From the routed PCB, which its z8002_bionic.toml does not match.
// #BUSREQ is tied high, #BUSACK is open and #MO loops back to #MI.
#define PIN_NS 37 /* P7.19 P37 */
#define PIN_RW 2  /* P9.04 P40 */
#define PIN_BW 3  /* P9.05 P41 */
/* R/#W and B/#W together, one read of GPIO9 */
#define PORT_RWBW 9
#define RWBW_gp 4
#define RWBW_gm 0x3
#define RWBW_vp 0
#define PIN_MREQ 4   /* P9.06 P42 */
#define PIN_DS 33    /* P9.07 P43 */
#define PIN_AS 5     /* P9.08 P46 */
#define PIN_CLOCK 29 /* P9.31 P47 */
#define PIN_NMI 6    /* P7.10 P50 */
#define PIN_NVI 9    /* P7.11 P51 */
#define PIN_VI 32    /* P7.12 P52 */
#define PIN_RESET 28 /* P8.18 P53 */
#define PIN_WAIT 31  /* P8.22 P54 */
#define PIN_STOP 30  /* P8.23 P55 */

#include "pins.h"
#include "signals_z8000.h"

namespace debugger {
namespace z8000 {

struct PinsZ8000 final : Pins {
    // The Z8001 runs the debugger segmented; its traps push four words.
    PinsZ8000(bool segmented);
    bool segmented() const { return _segmented; }
    // Words a trap pushes: the PC (two on the Z8001), the FCW and the
    // identifier.
    uint_fast8_t frameWords() const { return _segmented ? 4 : 3; }

    void idle() override;
    bool step(bool show) override;
    void run() override;

    void printCycles() override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;
    void setBreakInst(uint32_t addr) const override;

    // Where an injected sequence leaves off when not told: past its end.
    static constexpr uint32_t EXIT_END = UINT32_MAX;

    // Run |inst| on the CPU parked at |org|: a read in the window
    // [org, org+len) is answered from |inst|, a write captured into |buf|
    // and kept from memory. It ends at the read of |exit| once the
    // window's last word was read and |max| bytes captured, and parks
    // there; |org| becomes where.
    void execInst(const uint8_t *inst, uint_fast8_t len, uint32_t &org,
            uint32_t exit = EXIT_END);
    void captureWrites(const uint8_t *inst, uint_fast8_t len, uint8_t *buf,
            uint_fast8_t max, uint32_t &org, uint32_t exit = EXIT_END);

private:
    const bool _segmented;
    bool _resumed = false;  // the cycle comes back out of #WAIT
    void resetPins() override;
    const SignalsImpl *findBacktraceStart() override;
    void printBacktrace() override;

    Signals *prepareCycle();
    Signals *resumeCycle(uint32_t addr);
    Signals *completeCycle(Signals *s);
    void storeData(Signals *s);
    void execute(const uint8_t *inst, uint_fast8_t len, uint8_t *buf,
            uint_fast8_t max, uint32_t &org, uint32_t exit);
    bool scBreak(Signals *id);
    bool parkAfterFrame(Signals *push, Signals *from);
    bool suspend(Signals *s);
    bool rawStep();
    bool loop();
};

}  // namespace z8000
}  // namespace debugger
#endif /* __PINS_Z8000_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
