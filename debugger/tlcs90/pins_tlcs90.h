#ifndef _PINS_TLCS90_H__
#define _PINS_TLCS90_H__

#define PORT_DB 6    /* GPIO6 */
#define DB_gp 16     /* P6.16-P6.23 */
#define DB_gm 0xFF   /* P6.16-P6.23 */
#define DB_vp 0      /* D0-D7 */
#define PIN_DB0 19   /* P6.16 */
#define PIN_DB1 18   /* P6.17 */
#define PIN_DB2 14   /* P6.18 */
#define PIN_DB3 15   /* P6.19 */
#define PIN_DB4 40   /* P6.20 */
#define PIN_DB5 41   /* P6.21 */
#define PIN_DB6 17   /* P6.22 */
#define PIN_DB7 16   /* P6.23 */
#define PORT_ADL 6   /* GPIO6 */
#define ADL_gp 24    /* P6.24-P6.31 */
#define ADL_gm 0xFF  /* P6.24-P6.31 */
#define ADL_vp 0     /* A0-A7 */
#define PIN_AD0 22   /* P6.24 */
#define PIN_AD1 23   /* P6.25 */
#define PIN_AD2 20   /* P6.26 */
#define PIN_AD3 21   /* P6.27 */
#define PIN_AD4 38   /* P6.28 */
#define PIN_AD5 39   /* P6.29 */
#define PIN_AD6 26   /* P6.30 */
#define PIN_AD7 27   /* P6.31 */
#define PORT_ADM 7   /* GPIO7 */
#define ADM_gp 0     /* P7.00-P7.03 */
#define ADM_gm 0xF   /* P7.00-P7.03 */
#define ADM_vp 8     /* A8-A11 */
#define PIN_AD8 10   /* P7.00 */
#define PIN_AD9 12   /* P7.01 */
#define PIN_AD10 11  /* P7.02 */
#define PIN_AD11 13  /* P7.03 */
#define PORT_ADH 7   /* GPIO7 */
#define ADH_gp 16    /* P7.16-P7.19 */
#define ADH_gm 0xF   /* P7.16-P7.19 */
#define ADH_vp 12    /* A12-A14 */
#define PIN_AD12 8   /* P7.16 */
#define PIN_AD13 7   /* P7.17 */
#define PIN_AD14 36  /* P7.18 */
#define PIN_AD15 37  /* P6.19 */
#define PORT_CNTL 9  /* GPIO9 */
#define CNTL_gp 4    /* P9.04-P9.05 */
#define CNTL_gm 0x3  /* P9.04-P9.05 */
#define CNTL_vp 0    /* CNTL0-CNTL1 */
#define PIN_RD 2     /* P9.04 */
#define PIN_WR 3     /* P9.05 */
#define CNTL_RD 0x1  /* CNTL0 */
#define CNTL_WR 0x2  /* CNTL1 */
#define PIN_TXD 0    /* P6.03 */
#define PIN_RXD 1    /* P6.04 */
#define PIN_X1 5     /* P9.08 */
#define PIN_CLK 29   /* P9.31 */
#define PIN_INT0 6   /* P7.10 */
#define PIN_NMI 9    /* P7.11 */
#define PIN_WAIT 32  /* P7.12 */
#define PIN_RESET 28 /* P8.18 */
#define PIN_RTS 31   /* P8.22 */
#define PIN_INT1 30  /* P8.23 */

#include "pins.h"
#include "signals_tlcs90.h"

namespace debugger {
namespace tlcs90 {

enum IntrName : uint8_t {
    INTR_NONE = 0,
    INTR_INT0 = 0x28,
    INTR_INT1 = 0x58,
};

struct PinsTlcs90 final : Pins {
    void reset() override;
    void idle() override;
    bool step(bool show) override;
    void run() override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;
    void printCycles() override { printCycles(nullptr); }

    void execInst(const uint8_t *inst, uint8_t len);
    void captureWrites(const uint8_t *inst, uint8_t len, uint8_t *buf,
            uint8_t max, uint16_t *addr = nullptr);

private:
    void setBreakInst(uint32_t addr) const override;

    Signals *prepareCycle();
    Signals *completeCycle(Signals *signals);
    void suspend();
    void loop();
    void execute(const uint8_t *inst, uint8_t len, uint8_t *buffer, uint8_t max,
            uint16_t *addr);

    void printCycles(const Signals *end);
    bool matchAll(Signals *begin, const Signals *end);
    const Signals *findFetch(Signals *begin, const Signals *end);
    void disassembleCycles();
};

extern struct PinsTlcs90 Pins;

}  // namespace tlcs90
}  // namespace debugger
#endif /* _PINS_TLCS90_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
