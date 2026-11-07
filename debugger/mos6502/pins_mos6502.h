#ifndef __PINS_MOS6502_H__
#define __PINS_MOS6502_H__

#define PORT_D 6      /* GPIO6 */
#define D_gp 16       /* P6.16-P6.23 */
#define D_gm 0xFF     /* P6.16-P6.23 */
#define D_vp 0        /* D0-D7 */
#define PIN_D0 19     /* P6.16 */
#define PIN_D1 18     /* P6.17 */
#define PIN_D2 14     /* P6.18 */
#define PIN_D3 15     /* P6.19 */
#define PIN_D4 40     /* P6.20 */
#define PIN_D5 41     /* P6.21 */
#define PIN_D6 17     /* P6.22 */
#define PIN_D7 16     /* P6.23 */
#define PORT_BA 6     /* GPIO6 */
#define BA_gp 16      /* P6.16-P6.23 */
#define BA_gm 0xFF    /* P6.16-P6.23 */
#define BA_vp 16      /* A16-A23 */
#define PORT_AL 6     /* GPIO6 */
#define AL_gp 24      /* P6.24-P6.31 */
#define AL_gm 0xFF    /* P6.24-P6.31 */
#define AL_vp 0       /* A0-A7 */
#define PIN_AL0 22    /* P6.24 */
#define PIN_AL1 23    /* P6.25 */
#define PIN_AL2 20    /* P6.26 */
#define PIN_AL3 21    /* P6.27 */
#define PIN_AL4 38    /* P6.28 */
#define PIN_AL5 39    /* P6.29 */
#define PIN_AL6 26    /* P6.30 */
#define PIN_AL7 27    /* P6.31 */
#define PORT_AM 7     /* GPIO7 */
#define AM_gp 0       /* P7.00-P7.03 */
#define AM_gm 0xF     /* P7.00-P7.03 */
#define AM_vp 8       /* A8-A11 */
#define PIN_AM8 10    /* P7.00 */
#define PIN_AM9 12    /* P7.01 */
#define PIN_AM10 11   /* P7.02 */
#define PIN_AM11 13   /* P7.03 */
#define PORT_AH 7     /* GPIO7 */
#define AH_gp 16      /* P7.16-P7.19 */
#define AH_gm 0xF     /* P7.16-P7.19 */
#define AH_vp 12      /* A12-A15 */
#define PIN_AH12 8    /* P7.16 */
#define PIN_AH13 7    /* P7.17 */
#define PIN_AH14 36   /* P7.18 */
#define PIN_AH15 37   /* P7.19 */
#define PORT_CNTL 9   /* GPIO9 */
#define CNTL_gp 4     /* P9.04-P9.08 */
#define CNTL_gm 0x1F  /* P9.04-P9.08 */
#define CNTL_vp 0     /* CNTL0-CNTL4 */
#define PIN_VP 2      /* P9.04 */
#define PIN_RW 3      /* P9.05 */
#define PIN_SYNC 4    /* P9.06 */
#define PIN_PHI2O 33  /* P9.07 */
#define PIN_E 5       /* P9.08 */
#define CNTL_VP 0x1   /* CNTL0 */
#define CNTL_RW 0x2   /* CNTL1 */
#define CNTL_SYNC 0x4 /* CNTL2 */
#define CNTL_VPD 0x8  /* CNTL3 */
#define CNTL_E 0x10   /* CNTL4 */
#define PIN_ML 0      /* P6.03 */
#define PIN_SO 1      /* P6.02 */
#define PIN_PHI1O 29  /* P9.31 */
#define PIN_IRQ 6     /* P7.10 */
#define PIN_NMI 9     /* P7.11 */
#define PIN_RDY 32    /* P7.12 */
#define PIN_RES 28    /* P8.18 */
#define PIN_BE 31     /* P8.22 */
#define PIN_PHI0 30   /* P8.23 */

// Pin alias for W65C816
#define W65C816_VPA PIN_SYNC
#define W65C816_VDA PIN_PHI2O
#define W65C816_ABORT PIN_PHI1O
#define W65C816_MX PIN_SO
#define CNTL_VPA CNTL_SYNC

#include <Arduino.h>
#include "digital_bus.h"
#include "inst_mos6502.h"
#include "pins.h"
#include "signals_mos6502.h"

namespace debugger {
namespace mos6502 {

struct PinsMos6502 final : Pins {
    PinsMos6502();

    void resetPins() override;
    void idle() override;
    bool step(bool show) override;
    void run() override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;
    void setBreakInst(uint32_t addr) const override;
    void printCycles() override;

    void execInst(const uint8_t *inst, uint8_t len);
    void captureWrites(const uint8_t *inst, uint8_t len, uint16_t *addr,
            uint8_t *buf, uint8_t max);

    Signals *inject(uint8_t data);
    bool native65816() const;
#ifdef TUNE_CLOCK
    uint32_t clockInput() const override;
    bool setClockInput(uint32_t hz) override;
#endif

    // An engine's delays in ns.
    struct Timing {
        uint16_t lo;          // PHI0 fall to address and control
        uint16_t loSync;      // control to address, 16-bit cycles
        uint16_t loPoll;      // run loop, no device serviced
        uint16_t loFetch;     // run loop
        uint16_t loLoop;      // run loop, not a fetch
        uint16_t loExecute;   // execute()'s cycles
        uint16_t hiReadPre;   // PHI0 rise or memory read to data out
        uint16_t hiReadPost;  // data out to PHI0 fall
        uint16_t hiInject;    // in place of a memory read
        uint16_t hiWrite;     // PHI0 rise to write data
        uint16_t hiCapture;   // in place of a memory write
    };

    // What the NMI that stopped the CPU stacked: PBR (native), PCH, PCL,
    // P, and the S they were pushed from; len is 0 without one.
    struct Frame {
        uint8_t len;
        uint16_t sp;
        uint8_t data[4];
    };
    const Frame &frame() const { return _frame; }
    void clearFrame() { _frame.len = 0; }
    static HardwareType hardwareType() { return _hardType; }
    static SoftwareType softwareType() { return _softType; }

private:
    static HardwareType _hardType;
    static SoftwareType _softType;
    Frame _frame = {};
    // The engine bus cycles run on, one per SoftwareType: the hardware
    // type's own (HW_ and SW_ values coincide) until the software type is
    // known.
    SoftwareType _engine = SW_MOS6502;
    // What sets each engine's cycles apart, as constants.
    template <SoftwareType E>
    struct Traits;
#ifdef TUNE_CLOCK
    // The delays K scales: per mille of the part's rated clock.
    uint16_t _clockPermille = 1000;
    Timing _timing;
    void setTiming();
#endif

    void checkHardwareType();
    template <SoftwareType E>
    void checkSoftwareType();
    bool stopInst(uint8_t inst) const;
    void setEngine(SoftwareType engine);
    template <SoftwareType E>
    void resetCpu();
    // Waits the engine's |F| delay; one the datasheet leaves at 0 costs
    // nothing.
    template <SoftwareType E, uint16_t Timing::*F>
    void delay() const;
    template <typename F>
    auto withEngine(F f);

    friend struct RegsMos6502;
    template <SoftwareType E>
    Signals *prepareCycle();
    template <SoftwareType E>
    Signals *rawPrepareCycle();
    template <SoftwareType E>
    Signals *completeCycle(Signals *signals);
    template <SoftwareType E>
    Signals *cycle();
    template <SoftwareType E>
    Signals *inject(uint8_t data);
    template <SoftwareType E>
    bool suspend();
    template <SoftwareType E>
    bool loop();
    template <SoftwareType E>
    bool rawStep();
    template <SoftwareType E>
    void execute(const uint8_t *inst, uint8_t len, uint16_t *addr, uint8_t *buf,
            uint8_t max);
    void execute(const uint8_t *inst, uint8_t len, uint16_t *addr, uint8_t *buf,
            uint8_t max);

    const SignalsImpl *findBacktraceStart() override;
    void printBacktrace() override;
};

void Signals::getControl() {
    cntl() = busRead(CNTL);
}

// Each port is read once: AM and AH share GPIO7, AL and BA GPIO6.
void Signals::getAddr16() {
    const auto p7 = PIN(AM);
    addr = busRead(AL) | busValue(AM, p7) | busValue(AH, p7);
}

void Signals::getAddr24() {
    const auto p6 = PIN(AL);
    const auto p7 = PIN(AM);
    addr = busValue(AL, p6) | busValue(BA, p6) | busValue(AM, p7) |
           busValue(AH, p7);
}

void Signals::getData() {
    data = busRead(D);
}

void Signals::stageData() const {
    busWrite(D, data);
}

void Signals::outputMode() {
    PDDR(D) = _dataOutput;
}

void Signals::inputMode() {
    PDDR(D) = _dataInput;
}

bool Signals::read() const {
    return cntl() & CNTL_RW;
}

bool Signals::write() const {
    return (cntl() & CNTL_RW) == 0;
}

bool Signals::addr24() const {
    return (cntl() & CNTL_E) == 0;
}

template <HardwareType H>
bool Signals::fetch() const {
    // The W65C816 has no SYNC; VPA and VDA (VPD) both high mark a fetch.
    if constexpr (H == HW_W65C816)
        return (cntl() & (CNTL_VPA | CNTL_VPD)) == (CNTL_VPA | CNTL_VPD);
    return cntl() & CNTL_SYNC;
}

template <HardwareType H>
bool Signals::vector() const {
    // The 6502 pinout has no VP.
    if constexpr (H == HW_MOS6502)
        return read() && addr >= InstMos6502::VECTOR_NMI;
    return (cntl() & CNTL_VP) == 0;
}

}  // namespace mos6502
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
