#ifndef __PINS_I8051_H__
#define __PINS_I8051_H__

#define PORT_DB 6     /* GPIO6 */
#define DB_gp 16      /* P6.16-P6.23 */
#define DB_gm 0xFF    /* P6.16-P6.23 */
#define DB_vp 0       /* AD0-AD7 */
#define PIN_D0 19     /* P6.16 */
#define PIN_D1 18     /* P6.17 */
#define PIN_D2 14     /* P6.18 */
#define PIN_D3 15     /* P6.19 */
#define PIN_D4 40     /* P6.20 */
#define PIN_D5 41     /* P6.21 */
#define PIN_D6 17     /* P6.22 */
#define PIN_D7 16     /* P6.23 */
#define PORT_AD 6     /* GPIO6 */
#define AD_gp 16      /* P6.16-P6.31 */
#define AD_gm 0xFFFF  /* P6.16-P6.31 */
#define AD_vp 0       /* A0-A11 */
#define PIN_AD0 19    /* P6.16 */
#define PIN_AD1 18    /* P6.17 */
#define PIN_AD2 14    /* P6.18 */
#define PIN_AD3 15    /* P6.19 */
#define PIN_AD4 40    /* P6.20 */
#define PIN_AD5 41    /* P6.21 */
#define PIN_AD6 17    /* P6.22 */
#define PIN_AD7 16    /* P6.23 */
#define PIN_AD8 22    /* P6.24 */
#define PIN_AD9 23    /* P6.25 */
#define PIN_AD10 20   /* P6.26 */
#define PIN_AD11 21   /* P6.27 */
#define PIN_AD12 38   /* P6.27 */
#define PIN_AD13 39   /* P6.27 */
#define PIN_AD14 26   /* P6.27 */
#define PIN_AD15 27   /* P6.27 */
#define PIN_P10 10    /* P7.00 */
#define PIN_P11 12    /* P7.01 */
#define PIN_P12 11    /* P7.02 */
#define PIN_P13 13    /* P7.03 */
#define PIN_P14 8     /* P7.16 */
#define PIN_P15 7     /* P7.17 */
#define PIN_P16 36    /* P7.18 */
#define PIN_P17 37    /* P7.19 */
#define PORT_CNTL 9   /* GPIO9 */
#define CNTL_gp 4     /* P9.04-P9.06 */
#define CNTL_gm 0x7   /* P9.04-P9.06 */
#define CNTL_vp 0     /* CNTL0-CNTL2 */
#define PIN_PSEN 2    /* P9.04 */
#define PIN_RD 3      /* P9.05 */
#define PIN_WR 4      /* P9.06 */
#define CNTL_PSEN 0x1 /* CNTL0 */
#define CNTL_RD 0x2   /* CNTL1 */
#define CNTL_WR 0x4   /* CNTL2 */
#define PIN_ALE 33    /* P9.07 */
#define PIN_TXD 0     /* P6.03 */
#define PIN_RXD 1     /* P6.02 */
#define PIN_XTYPE 5   /* P9.08 */
#define PIN_XTAL 29   /* P9.31 */
#define PIN_INT0 6    /* P7.10 */
#define PIN_INT1 9    /* P7.11 */
#define PIN_EA 32     /* P7.12 */
#define PIN_RST 28    /* P8.18 */
#define PIN_T0 31     /* P8.22 */
#define PIN_T1 30     /* P8.23 */

#include "pins.h"
#include "signals_i8051.h"

namespace debugger {
namespace i8051 {

enum IntrName : uint8_t {
    INTR_NONE = 0,     // ORG_RESET
    INTR_INT0 = 0x03,  // ORG_IE0
    INTR_INT1 = 0x13,  // ORG_IE1
};

struct PinsI8051 final : Pins {
    void reset() override;
    void idle() override;
    bool step(bool show) override;
    void run() override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;
    void setBreakInst(uint32_t addr) const override;
    void printCycles() override;

    void execInst(const uint8_t *inst, uint8_t len);
    uint8_t captureWrites(const uint8_t *inst, uint8_t len, uint16_t *addr,
            uint8_t *buf, uint8_t max);

    bool isCmos() const;

private:
    void (*_xtal_lo)();
    void (*_xtal_hi)();
    void xtal_lo() const { _xtal_lo(); }
    void xtal_hi() const { _xtal_hi(); }
    void xtal_cycle_lo() const;
    void xtal_cycle() const;
    Signals *prepareCycle();
    Signals *completeCycle(Signals *signals);
    void loop();
    Signals *inject(uint8_t data);
    void inject(const uint8_t data[], uint8_t len);
    bool rawStep();
    uint8_t execute(const uint8_t *inst, uint8_t len, uint16_t *addr,
            uint8_t *buf, uint8_t max);

    void disassembleCycles();
};

extern struct PinsI8051 Pins;

}  // namespace i8051
}  // namespace debugger
#endif /* __PINS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
