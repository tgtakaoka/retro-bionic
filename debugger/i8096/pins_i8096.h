#ifndef __PINS_I8096_H__
#define __PINS_I8096_H__

#define PORT_DATA 6     /* GPIO6 */
#define DATA_gp 16      /* P6.16-P6.31 */
#define DATA_gm 0xFF    /* P6.16-P6.23 */
#define DATA_vp 0       /* D0-D7 */
#define PORT_AD 6       /* GPIO6 */
#define AD_gp 16        /* P6.16-P6.31 */
#define AD_gm 0xFFFF    /* P6.16-P6.31 */
#define AD_vp 0         /* A0-A15 */
#define PIN_AD0 19      /* P6.16 */
#define PIN_AD1 18      /* P6.17 */
#define PIN_AD2 14      /* P6.18 */
#define PIN_AD3 15      /* P6.19 */
#define PIN_AD4 40      /* P6.20 */
#define PIN_AD5 41      /* P6.21 */
#define PIN_AD6 17      /* P6.22 */
#define PIN_AD7 16      /* P6.23 */
#define PIN_AD8 22      /* P6.24 */
#define PIN_AD9 23      /* P6.25 */
#define PIN_AD10 20     /* P6.26 */
#define PIN_AD11 21     /* P6.27 */
#define PIN_AD12 38     /* P6.28 */
#define PIN_AD13 39     /* P6.29 */
#define PIN_AD14 26     /* P6.30 */
#define PIN_AD15 27     /* P6.31 */
#define PIN_ADV 2       /* P9.04 */
#define PIN_RD 3        /* P9.05 */
#define PIN_WR 4        /* P9.06 */
#define PIN_BHE 33      /* P9.07 */
#define CNTL_ADV 0x1    /* CNTL0 */
#define CNTL_RD 0x2     /* CNTL1 */
#define CNTL_WR 0x4     /* CNTL2 */
#define CNTL_BHE 0x8    /* CNTL3 */
#define CNTL_INST 0x10   /* CNTL4: an instruction fetch, where the CPU has it */
#define CNTL_START0 0x40 /* an instruction starts at the cycle's address */
#define CNTL_START1 0x80 /* one starts at the byte after it */
#define PIN_RESET 28    /* P8.18 */
#define PIN_READY 31    /* P8.22 */
#define PIN_EXTINT 30   /* P8.23 */
#define PORT_HSO 7      /* GPIO7 */
#define HSO_gp 0        /* P7.00-P7.03 */
#define HSO_gm 0xF      /* P7.00-P7.03 */
#define HSO_vp 0        /* HSO0-HSO3 */
#define PIN_HSO0 10     /* P7.00 */
#define PIN_HSO1 12     /* P7.01 */
#define PIN_HSO2 11     /* P7.02 */
#define PIN_HSO3 13     /* P7.03 */
#define PORT_HSI 7      /* GPIO7 */
#define HSI_gp 16       /* P7.16-P7.19 */
#define HSI_gm 0xF      /* P7.16-P7.19 */
#define HSI_vp 4        /* HSI0-HSI3 */
#define PIN_HSI0 8      /* P7.16 */
#define PIN_HSI1 7      /* P7.17 */
#define PIN_HSI2 36     /* P7.18 */
#define PIN_HSI3 37     /* P7.19 */
#define PIN_TXD 0       /* P6.03 */
#define PIN_RXD 1       /* P6.02 */
#define PIN_XTAL1 29    /* P9.31 */

#include "inst_i8096.h"
#include "pins.h"
#include "signals_i8096.h"

namespace debugger {
namespace i8096 {

// The board the P8095BH, the N8097BH and the S80C196KB/KC sit on; each
// adds its own pins and control lines.
struct PinsI8096 : Pins {
    void idle() override;
    bool step(bool show) override;
    void run() override;

    void printCycles() override { printCycles(nullptr); }
    void assertInt(uint8_t name = 0) override;
    void negateInt(uint8_t name = 0) override;
    void setBreakInst(uint32_t addr) const override;

    CpuType cpuType() const { return _cpu; }
    // Whether the CPU is an 80C196: PUSHA, POPA, INT_MASK1 and WSR.
    bool c196() const { return _cpu >= CPU_80C196KB; }
    // Where the CPU is parked: the address of its next fetch.
    uint16_t park() const { return _park; }
    // Where an injected sequence leaves off when not told: at the park.
    static constexpr uint32_t EXIT_PARK = UINT32_MAX;
    // Run |inst| from the park: a read in the window [park, park+len) is
    // answered from |inst|, one past it with NOPs, and a write captured
    // into |buf|, up to |max| bytes, and kept from memory. It ends at the
    // read of |exit| once the window's last byte was read and |max| bytes
    // captured, and parks there. Returns the first write's address.
    uint16_t execInst(const uint8_t *inst, uint_fast8_t len,
            uint8_t *buf = nullptr, uint_fast8_t max = 0,
            uint32_t exit = EXIT_PARK);
    // As execInst() to |exit|, and a read at [at, at+|size|) is answered
    // from |data|: the words a POP pulls.
    void popInst(const uint8_t *inst, uint_fast8_t len, uint16_t at,
            const uint8_t *data, uint_fast8_t size, uint32_t exit);

protected:
    using Signals = SignalsI8096;

    explicit PinsI8096(CpuType cpu = CPU_8096);
    void resetPins() override;
    // Reads the control lines into |s|: whether a read or a write is on.
    virtual bool getControl(Signals *s) const = 0;

private:
    const CpuType _cpu;
    bool _idle = false;
    // The CPU waits in the read of |_park|, a copy of its cycle kept.
    bool _held = false;
    uint16_t _park;
    Signals _idleSignals;
    Signals _heldSignals;

    bool rawStep(bool show);
    Signals *loop();

    Signals *prepareCycle();
    Signals *completeCycle(Signals *s, bool low = false);
    Signals *noBusCycle(Signals *s);
    void hold(const Signals *s, uint16_t park);
    uint16_t readBus(const Signals *s) const;
    void writeBus(const Signals *s) const;
    uint16_t execute(uint16_t org, const uint8_t *inst, uint_fast8_t len,
            uint8_t *buf, uint_fast8_t max, uint32_t exit, bool idle,
            uint16_t at = 0, const uint8_t *data = nullptr,
            uint_fast8_t size = 0);
    bool fetchedBreak(const Signals *s) const;
    uint16_t jumpTarget(uint16_t next, uint_fast8_t opc) const;
    uint16_t readData16(uint16_t addr) const;
    void handleTrap(Signals *s, uint16_t vector, bool breakTrap);

    void printCycles(const Signals *end);
    const Signals *findFetch(Signals *begin, const Signals *end);
#ifdef PROFILE_CYCLES
    const Signals *_profileEnd = nullptr;  // the TRAP's last cycle, + 1
#endif
    const SignalsImpl *findBacktraceStart() override;
    void printBacktrace() override;
};

}  // namespace i8096
}  // namespace debugger
#endif /* __PINS_I8096_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
