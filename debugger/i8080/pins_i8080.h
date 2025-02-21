#ifndef __PINS_I8080_H__
#define __PINS_I8080_H__

#define PORT_DATA 6   /* GPIO6 */
#define DATA_gp 16    /* P6.16-P6.23 */
#define DATA_gm 0xFF  /* P6.16-P6.23 */
#define DATA_vp 0     /* AD0-AD7 */
#define PIN_D0 19     /* P6.16 */
#define PIN_D1 18     /* P6.17 */
#define PIN_D2 14     /* P6.18 */
#define PIN_D3 15     /* P6.19 */
#define PIN_D4 40     /* P6.20 */
#define PIN_D5 41     /* P6.21 */
#define PIN_D6 17     /* P6.22 */
#define PIN_D7 16     /* P6.23 */
#define PORT_ADRL 6   /* GPIO6 */
#define ADRL_gp 24    /* P6.24-P6.31 */
#define ADRL_gm 0xFF  /* P6.24-P6.31 */
#define ADRL_vp 0     /* A0-A7 */
#define PIN_ADDR0 22  /* P6.24 */
#define PIN_ADDR1 23  /* P6.25 */
#define PIN_ADDR2 20  /* P6.26 */
#define PIN_ADDR3 21  /* P6.27 */
#define PIN_ADDR4 38  /* P6.28 */
#define PIN_ADDR5 39  /* P6.29 */
#define PIN_ADDR6 26  /* P6.30 */
#define PIN_ADDR7 27  /* P6.31 */
#define PORT_ADRM 7   /* GPIO7 */
#define ADRM_gp 0     /* P7.00-P7.03 */
#define ADRM_gm 0xF   /* P7.00-P7.03 */
#define ADRM_vp 8     /* A8-A11 */
#define PIN_ADDR8 10  /* P7.00 */
#define PIN_ADDR9 12  /* P7.01 */
#define PIN_ADDR10 11 /* P7.02 */
#define PIN_ADDR11 13 /* P7.03 */
#define PORT_ADRH 7   /* GPIO7 */
#define ADRH_gp 16    /* P7.16-P7.19 */
#define ADRH_gm 0xF   /* P7.16-P7.19 */
#define ADRH_vp 12    /* A12-A15 */
#define PIN_ADDR12 8  /* P7.16 */
#define PIN_ADDR13 7  /* P7.17 */
#define PIN_ADDR14 36 /* P7.18 */
#define PIN_ADDR15 37 /* P7.19 */
#define PORT_CNTL 9   /* GPIO9 */
#define CNTL_gp 4     /* P9.04-P9.07 */
#define CNTL_gm 0xF   /* P9.04-P9.07 */
#define CNTL_vp 0     /* CNTL0-CNTL3 */
#define PIN_DBIN 2    /* P9.04 */
#define PIN_WR 3      /* P9.05 */
#define PIN_SYNC 4    /* P9.06 */
#define PIN_HLDA 33   /* P9.07 */
#define CNTL_DBIN 1   /* CNTL0 */
#define CNTL_WR 2     /* CNTL1 */
#define CNTL_SYNC 4   /* CNTL2 */
#define CNTL_HLDA 8   /* CNTL3 */
#define PIN_PHI1 5    /* P9.31 */
#define PIN_PHI2 29   /* P9.08 */
#define PIN_INT 6     /* P7.10 */
#define PIN_INTE 9    /* P7.11 */
#define PIN_HOLD 32   /* P7.12 */
#define PIN_RESET 28  /* P8.18 */
#define PIN_READY 31  /* P8.22 */
#define PIN_WAIT 30   /* P8.23 */

#include "pins_i8080_base.h"
#include "signals_i8080.h"

namespace debugger {
namespace i8080 {

enum IntrName : uint8_t {
    INTR_NONE = 0,
    INTR_RST1 = 0x08,  // RST 1: 0008H
    INTR_RST2 = 0x10,  // RST 2: 0010H
    INTR_RST3 = 0x18,  // RST 3: 0018H
    INTR_RST4 = 0x20,  // RST 4: 0020H
    INTR_RST5 = 0x28,  // RST 5: 0028H
    INTR_RST6 = 0x30,  // RST 6: 0030H
    INTR_RST7 = 0x38,  // RST 7: 0038H
};

struct PinsI8080 final : PinsI8080Base {
    PinsI8080();

    void resetPins() override;
    void idle() override;
    bool step(bool show) override;
    void run() override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;
    void printCycles() override;

    uint16_t captureWrites(const uint8_t *inst, uint_fast8_t len, uint8_t *buf,
            uint_fast8_t max) override;

private:
    Signals *prepareCycle() const;
    void enterWait() const;
    Signals *resumeCycle(uint16_t pc = 0) const;
    Signals *completeCycle(Signals *s) const;
    Signals *inject(uint8_t data) const;
    void loop() const;
    bool rawStep() const;

    void disassembleCycles();
};

}  // namespace i8080
}  // namespace debugger
#endif /* __PINS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
