#ifndef __PINS_I8085_H__
#define __PINS_I8085_H__

#define PORT_AD 6      /* GPIO6 */
#define AD_gp 16       /* P6.16-P6.23 */
#define AD_gm 0xFF     /* P6.16-P6.23 */
#define AD_vp 0        /* AD0-AD7 */
#define PIN_AD0 19     /* P6.16 */
#define PIN_AD1 18     /* P6.17 */
#define PIN_AD2 14     /* P6.18 */
#define PIN_AD3 15     /* P6.19 */
#define PIN_AD4 40     /* P6.20 */
#define PIN_AD5 41     /* P6.21 */
#define PIN_AD6 17     /* P6.22 */
#define PIN_AD7 16     /* P6.23 */
#define PORT_ADDR 6    /* GPIO6 */
#define ADDR_gp 16     /* P6.16-P6.31 */
#define ADDR_gm 0xFFFF /* P6.16-P6.31 */
#define ADDR_vp 0      /* A0-A15 */
#define PIN_ADDR8 22   /* P6.24 */
#define PIN_ADDR9 23   /* P6.25 */
#define PIN_ADDR10 20  /* P6.26 */
#define PIN_ADDR11 21  /* P6.27 */
#define PIN_ADDR12 38  /* P6.28 */
#define PIN_ADDR13 39  /* P6.29 */
#define PIN_ADDR14 26  /* P6.30 */
#define PIN_ADDR15 27  /* P6.31 */
#define PIN_RST55 10   /* P7.00 */
#define PIN_RST65 12   /* P7.01 */
#define PIN_RST75 11   /* P7.02 */
#define PORT_S 7       /* GPIO7 */
#define S_gp 16        /* P7.16-P7.19 */
#define S_gm 0x3       /* P7.16-P7.19 */
#define S_vp 0         /* S0-S1 */
#define PIN_S0 8       /* P7.16 */
#define PIN_S1 7       /* P7.17 */
#define PIN_HLDA 36    /* P7.18 */
#define PIN_HOLD 37    /* P7.19 */
#define PIN_ALE 2      /* P9.04 */
#define PIN_INTR 4     /* P9.06 */
#define PIN_TRAP 33    /* P9.06 */
#define PIN_SOD 0      /* P6.03 */
#define PIN_SID 1      /* P6.02 */
#define PIN_X1 5       /* P9.08 */
#define PIN_CLK 29     /* P9.09 */
#define PIN_RD 6       /* P7.10 */
#define PIN_WR 9       /* P7.11 */
#define PIN_READY 32   /* P7.12 */
#define PIN_RESET 28   /* P8.18 */
#define PIN_INTA 31    /* P8.22 */
#define PIN_IOM 30     /* P8.23 */

#include "pins.h"
#include "signals_i8085.h"

namespace debugger {
namespace i8085 {

enum IntrName : uint8_t {
    INTR_NONE = 0,
    INTR_RST1 = 0x08,   // RST 1: 0008H
    INTR_RST2 = 0x10,   // RST 2: 0010H
    INTR_RST3 = 0x18,   // RST 3: 0018H
    INTR_RST4 = 0x20,   // RST 4: 0020H
    INTR_RST5 = 0x28,   // RST 5: 0028H
    INTR_RST6 = 0x30,   // RST 6: 0030H
    INTR_RST7 = 0x38,   // RST 7: 0038H
    INTR_RST55 = 0x2C,  // RST 5.5: 002CH
    INTR_RST65 = 0x34,  // RST 6.5: 0034H
    INTR_RST75 = 0x3C,  // RST 7.5: 003CH
    INTR_TRAP = 0x24,   // TRAP: 0024H
};

struct PinsI8085 final : Pins {
    void reset() override;
    void idle() override;
    bool step(bool show) override;
    void run() override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;
    void setBreakInst(uint32_t addr) const override;

    void execInst(const uint8_t *inst, uint8_t len);
    uint8_t captureWrites(const uint8_t *inst, uint8_t len, uint16_t *addr,
            uint8_t *buf, uint8_t max);

private:
    uint8_t _inta;

    Signals *cycleT1() const;
    Signals *cycleT2() const;
    Signals *cycleT2Pause() const;
    Signals *cycleT2Ready(uint16_t pc) const;
    Signals *cycleT3(Signals *signals);
    void loop();
    bool rawStep(uint16_t pc, bool step);
    uint8_t execute(const uint8_t *inst, uint8_t len, uint16_t *addr,
            uint8_t *buf, uint8_t max);

    void disassembleCycles();
    void printCycles();

    static uint8_t intrToInta(uint8_t name);
};

extern struct PinsI8085 Pins;

}  // namespace i8085
}  // namespace debugger
#endif /* __PINS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
