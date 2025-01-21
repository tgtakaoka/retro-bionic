#ifndef __PINS_MN1613_H__
#define __PINS_MN1613_H__

#define PORT_ADDR 6     /* GPIO6 */
#define ADDR_gp 16      /* P6.16-P6.31 */
#define ADDR_gm 0xFFFF  /* P6.16-P6.31 */
#define ADDR_vp 0       /* A15-A0 */
#define PORT_DATA 6     /* GPIO6 */
#define DATA_gp 16      /* P6.16-P6.31 */
#define DATA_gm 0xFFFF  /* P6.16-P6.31 */
#define DATA_vp 0       /* D15-D0 */
#define PIN_BS15 19     /* P6.16 */
#define PIN_BS14 18     /* P6.17 */
#define PIN_BS13 14     /* P6.18 */
#define PIN_BS12 15     /* P6.19 */
#define PIN_BS11 40     /* P6.20 */
#define PIN_BS10 41     /* P6.21 */
#define PIN_BS09 17     /* P6.22 */
#define PIN_BS08 16     /* P6.23 */
#define PIN_BS07 22     /* P6.24 */
#define PIN_BS06 23     /* P6.25 */
#define PIN_BS05 20     /* P6.26 */
#define PIN_BS04 21     /* P6.27 */
#define PIN_BS03 38     /* P6.28 */
#define PIN_BS02 39     /* P6.29 */
#define PIN_BS01 26     /* P6.30 */
#define PIN_BS00 27     /* P6.31 */
#define PORT_EA 7       /* GPIO7 */
#define EA_gp 0         /* P7.00-P7.03 */
#define EA_gm 0xF       /* P7.00-P7.03 */
#define EA_vp 16        /* EA3-EA0 */
#define PIN_EA3 10      /* P7.00 */
#define PIN_EA2 12      /* P7.01 */
#define PIN_EA1 11      /* P7.02 */
#define PIN_EA0 13      /* P7.03 */
#define PIN_CSTP 10     /* P7.00 */
#define PIN_FSYC 12     /* P7.01 */
#define PIN_RUN 8       /* P7.16 */
#define PIN_HLT 7       /* P7.17 */
#define PIN_STRT 36     /* P7.18 */
#define PIN_SYNC 37     /* P7.19 */
#define PORT_CNTL 9     /* GPIO9 */
#define CNTL_gp 4       /* P9.04-P9.08 */
#define CNTL_gm 0x1F    /* P9.04-P9.08 */
#define CNTL_vp 0       /* CNTL0-CNTL4 */
#define PIN_BSRQ 2      /* P9.04 */
#define PIN_ADSD 3      /* P9.05 */
#define PIN_DTSD 4      /* P9.06 */
#define PIN_WRT 33      /* P9.07 */
#define PIN_IOP 5       /* P9.08 */
#define CNTL_BSRQ 1     /* CNTL0 */
#define CNTL_ADSD 2     /* CNTL1 */
#define CNTL_DTSD 4     /* CNTL2 */
#define CNTL_WRT 8      /* CNTL3 */
#define CNTL_IOP 0x10   /* CNTL4 */
#define CNTL_FETCH 0x20 /* CNTL5 */
#define PIN_ST 0        /* P6.03 */
#define PIN_SD 1        /* P6.02 */
#define PIN_X2 29       /* P9.31 */
#define PIN_IRQ0 6      /* P7.10 */
#define PIN_IRQ1 9      /* P7.11 */
#define PIN_IRQ2 32     /* P7.12 */
#define PIN_RST 28      /* P8.18 */
#define PIN_DTAK 31     /* P8.22 */
#define PIN_BSAV 30     /* P8.23 */

#include "pins.h"
#include "signals_mn1613.h"

namespace debugger {
namespace mn1613 {

enum IntrName : uint8_t {
    INTR_IRQ0 = 1,  // OPSW0+1
    INTR_IRQ1 = 3,  // OPSW1+1
    INTR_IRQ2 = 5,  // OPSW2+1
};

struct PinsMn1613 final : Pins {
    PinsMn1613();

    void resetPins() override;
    void idle() override;
    bool step(bool show) override;
    void run() override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;
    void setBreakInst(uint32_t addr) const override;
    void printCycles() override;

    void injectReset(const uint16_t *data, uint_fast8_t len);
    void injectInst(uint16_t inst);
    void injectInst(const uint16_t *inst, uint_fast8_t len);
    void captureInst(uint16_t inst, uint16_t *buf);
    void injectReads(uint16_t inst, uint32_t addr, const uint16_t *data,
            uint_fast8_t len);

private:
    uint32_t _addr;

    void unhalt() const;
    void halt() const;
    Signals *waitReset() const;
    Signals *waitBus() const;
    Signals *prepareCycle(Signals *) const;
    Signals *completeCycle(Signals *s) const;
    void loop();
    bool rawStep();

    void disassembleCycles();
};

}  // namespace mn1613
}  // namespace debugger
#endif /* __PINS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
