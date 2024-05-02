#ifndef __PINS_I8048_H__
#define __PINS_I8048_H__

#define PORT_DB 6     /* GPIO6 */
#define DB_gp 16      /* P6.16-P6.23 */
#define DB_gm 0xFF    /* P6.16-P6.23 */
#define DB_vp 0       /* AD0-AD7 */
#define PIN_DB0 19    /* P6.16 */
#define PIN_DB1 18    /* P6.17 */
#define PIN_DB2 14    /* P6.18 */
#define PIN_DB3 15    /* P6.19 */
#define PIN_DB4 40    /* P6.20 */
#define PIN_DB5 41    /* P6.21 */
#define PIN_DB6 17    /* P6.22 */
#define PIN_DB7 16    /* P6.23 */
#define PORT_ADDR 6   /* GPIO6 */
#define ADDR_gp 16    /* P6.16-P6.27 */
#define ADDR_gm 0xFFF /* P6.16-P6.27 */
#define ADDR_vp 0     /* A0-A11 */
#define PIN_ADDR8 22  /* P6.24 */
#define PIN_ADDR9 23  /* P6.25 */
#define PIN_ADDR10 20 /* P6.26 */
#define PIN_ADDR11 21 /* P6.27 */
#define PIN_P24 38    /* P6.28 */
#define PIN_P25 39    /* P6.29 */
#define PIN_P26 26    /* P6.30 */
#define PIN_P27 27    /* P6.31 */
#define PIN_P10 10    /* P7.00 */
#define PIN_P11 12    /* P7.01 */
#define PIN_P12 11    /* P7.02 */
#define PIN_P13 13    /* P7.03 */
#define PIN_P14 8     /* P7.16 */
#define PIN_P15 7     /* P7.17 */
#define PIN_P16 36    /* P7.18 */
#define PIN_P17 37    /* P7.19 */
#define PORT_CNTL 9   /* GPIO9 */
#define CNTL_gp 4     /* P9.04-P9.07 */
#define CNTL_gm 0xF   /* P9.04-P9.07 */
#define CNTL_vp 0     /* CNTL0-CNTL3 */
#define PIN_PROG 2    /* P9.04 */
#define PIN_PSEN 3    /* P9.05 */
#define PIN_RD 4      /* P9.06 */
#define PIN_WR 33     /* P9.07 */
#define CNTL_PROG 0x1 /* CNTL0 */
#define CNTL_PSEN 0x2 /* CNTL1 */
#define CNTL_RD 0x4   /* CNTL2 */
#define CNTL_WR 0x8   /* CNTL3 */
#define PIN_T0 0      /* P6.03 */
#define PIN_T1 1      /* P6.02 */
#define PIN_XTAL1 5   /* P9.08 */
#define PIN_SS 32     /* P7.12 */
#define PIN_RESET 28  /* P8.18 */
#define PIN_INT 31    /* P8.22 */
#define PIN_ALE 30    /* P8.23 */

#include "pins.h"
#include "signals_i8048.h"

namespace debugger {
namespace i8048 {

struct RegsI8048;
struct ProgI8048;
struct InstI8048;

struct PinsI8048 final : Pins {
    PinsI8048(RegsI8048 &regs, ProgI8048 &mems, const InstI8048 &inst)
        : Pins(), _regs(regs), _mems(mems), _inst(inst) {}
    void reset() override;
    void idle() override;
    bool step(bool show) override;
    void run() override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;
    void printCycles() override;

    void execInst(const uint8_t *inst, uint8_t len);
    uint8_t captureWrites(const uint8_t *inst, uint8_t len, uint16_t *addr,
            uint8_t *buf, uint8_t max);
    Signals *cycle(uint8_t data);

private:
    RegsI8048 &_regs;
    ProgI8048 &_mems;
    const InstI8048 &_inst;

    void setBreakInst(uint32_t addr) const override;

    Signals *prepareCycle();
    Signals *completeCycle(Signals *signals);
    void loop();
    Signals *inject(uint8_t data);
    void injectJumpHere(Signals *signals);
    bool rawStep(bool step = false);
    uint8_t execute(const uint8_t *inst, uint8_t len, uint16_t *addr,
            uint8_t *buf, uint8_t max);

    void disassembleCycles();
};

}  // namespace i8048
}  // namespace debugger
#endif /* __PINS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
