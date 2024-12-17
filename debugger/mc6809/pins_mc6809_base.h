#ifndef __PINS_MC6809_BASE_H__
#define __PINS_MC6809_BASE_H__

#define PORT_D 6     /* GPIO6 */
#define D_gp 16      /* P6.16-P6.23 */
#define D_gm 0xFF    /* P6.16-P6.23 */
#define D_vp 0       /* D0-D7 */
#define PIN_D0 19    /* P6.16 */
#define PIN_D1 18    /* P6.17 */
#define PIN_D2 14    /* P6.18 */
#define PIN_D3 15    /* P6.19 */
#define PIN_D4 40    /* P6.20 */
#define PIN_D5 41    /* P6.21 */
#define PIN_D6 17    /* P6.22 */
#define PIN_D7 16    /* P6.23 */
#define PORT_AL 6    /* GPIO6 */
#define AL_gp 24     /* P6.24-P6.31 */
#define AL_gm 0xFF   /* P6.24-P6.31 */
#define AL_vp 0      /* A0-A7 */
#define PIN_AL0 22   /* P6.24 */
#define PIN_AL1 23   /* P6.25 */
#define PIN_AL2 20   /* P6.26 */
#define PIN_AL3 21   /* P6.27 */
#define PIN_AL4 38   /* P6.28 */
#define PIN_AL5 39   /* P6.29 */
#define PIN_AL6 26   /* P6.30 */
#define PIN_AL7 27   /* P6.31 */
#define PORT_AM 7    /* GPIO7 */
#define AM_gp 0      /* P7.00-P7.03 */
#define AM_gm 0xF    /* P7.12-P7.15 */
#define AM_vp 8      /* A8-A11 */
#define PIN_AM8 10   /* P7.00 */
#define PIN_AM9 12   /* P7.01 */
#define PIN_AM10 11  /* P7.02 */
#define PIN_AM11 13  /* P7.03 */
#define PORT_AH 7    /* P7.16-P7.19 */
#define AH_gp 16     /* P7.16-P7.19 */
#define AH_gm 0xF    /* P7.16-P7.19 */
#define AH_vp 12     /* A12-A15 */
#define PIN_AH12 8   /* P7.16 */
#define PIN_AH13 7   /* P7.17 */
#define PIN_AH14 36  /* P7.18 */
#define PIN_AH15 37  /* P7.19 */
#define PORT_CNTL 9  /* GPIO9 */
#define CNTL_gp 4    /* P9.04-P4.07 */
#define CNTL_gm 0xF  /* P9.04-P9.07 */
#define CNTL_vp 0    /* CNTL0-CNTL3 */
#define PIN_RW 3     /* P9.05 */
#define PIN_BS 4     /* P9.06 */
#define PIN_BA 33    /* P9.07 */
#define CNTL_VMA 0x1 /* CNTL0 */
#define CNTL_RW 0x2  /* CNTL1 */
#define CNTL_BS 0x4  /* CNTL2 */
#define CNTL_BA 0x8  /* CNTL3 */
#define PIN_FIRQ 1   /* P6.04 */
#define PIN_IRQ 6    /* P7.10 */
#define PIN_NMI 9    /* P7.11 */
#define PIN_RESET 28 /* P8.18 */
#define PIN_HALT 30  /* P8.23 */

#include "devs.h"
#include "inst_mc6809.h"
#include "mems.h"
#include "pins.h"
#include "signals_mc6809.h"

namespace debugger {
namespace mc6809 {

struct InstMc6809;
struct RegsMc6809;

enum IntrName : uint8_t {
    INTR_NONE = 0,
    INTR_IRQ = 1,
    INTR_FIRQ = 2,
};

struct PinsMc6809Base : Pins {
    ~PinsMc6809Base() { delete _inst; }

    void idle() override;
    bool step(bool show) override;
    void run() override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;
    void setBreakInst(uint32_t addr) const override;
    void printCycles() override { printCycles(nullptr); }

    void injectReads(const uint8_t *inst, uint8_t len, uint8_t cycles = 0);
    void captureWrites(uint8_t *buf, uint8_t len);
    uint8_t captureContext(uint8_t *context, uint16_t &sp);

protected:
    InstMc6809 *_inst;

    void resetPins() override;

    virtual Signals *rawCycle() const = 0;
    virtual Signals *cycle() const = 0;
    Signals *injectCycle(uint8_t data);
    void suspend(bool show);
    void loop();
    const Signals *stackFrame(const Signals *push) const;

    void printCycles(const Signals *end);
    bool matchAll(Signals *begin, const Signals *end);
    virtual const Signals *findFetch(Signals *begin, const Signals *end);
    void disassembleCycles();
};

}  // namespace mc6809
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
