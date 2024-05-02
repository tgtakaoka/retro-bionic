#ifndef __PINS_SCN2650_H__
#define __PINS_SCN2650_H__

#define PORT_DBUS 6      /* GPIO6 */
#define DBUS_gp 16       /* P6.16-P6.23 */
#define DBUS_gm 0xFF     /* P6.16-P6.23 */
#define DBUS_vp 0        /* DBUS0-DBUS7 */
#define PIN_DBUS0 19     /* P6.16 */
#define PIN_DBUS1 18     /* P6.17 */
#define PIN_DBUS2 14     /* P6.18 */
#define PIN_DBUS3 15     /* P6.19 */
#define PIN_DBUS4 40     /* P6.20 */
#define PIN_DBUS5 41     /* P6.21 */
#define PIN_DBUS6 17     /* P6.22 */
#define PIN_DBUS7 16     /* P6.23 */
#define PORT_ADRL 6      /* GPIO6 */
#define ADRL_gp 24       /* P6.24-P6.31 */
#define ADRL_gm 0xFF     /* P6.24-P6.31 */
#define ADRL_vp 0        /* ADR0-ADR7 */
#define PIN_ADR0 22      /* P6.24 */
#define PIN_ADR1 23      /* P6.25 */
#define PIN_ADR2 20      /* P6.26 */
#define PIN_ADR3 21      /* P6.27 */
#define PIN_ADR4 38      /* P6.28 */
#define PIN_ADR5 39      /* P6.29 */
#define PIN_ADR6 26      /* P6.30 */
#define PIN_ADR7 27      /* P6.31 */
#define PORT_ADRM 7      /* GPIO7 */
#define ADRM_gp 0        /* P7.00-P7.03 */
#define ADRM_gm 0xF      /* P7.00-P7.03 */
#define ADRM_vp 8        /* ADR8-ADR11 */
#define PIN_ADR8 10      /* P7.00 */
#define PIN_ADR9 12      /* P7.01 */
#define PIN_ADR10 11     /* P7.02 */
#define PIN_ADR11 13     /* P7.03 */
#define PORT_ADRH 7      /* GPIO7 */
#define ADRH_gp 16       /* P7.16-P7.18 */
#define ADRH_gm 0x7      /* P7.16-P7.18 */
#define ADRH_vp 12       /* ADR12-ADR14 */
#define PIN_ADR12 8      /* P7.16 */
#define PIN_ADR13 7      /* P7.17 */
#define PIN_ADR14 36     /* P7.18 */
#define PIN_PAUSE 37     /* P6.19 */
#define PORT_CNTL 9      /* GPIO9 */
#define CNTL_gp 4        /* P9.04~P9.07 */
#define CNTL_gm 0xF      /* P9.04~P9.07 */
#define CNTL_vp 4        /* CNTL4~CNTL7 */
#define PIN_MIO 2        /* P9.04 */
#define PIN_RW 3         /* P9.05 */
#define PIN_INTREQ 4     /* P9.06 */
#define PIN_INTACK 33    /* P9.07 */
#define CNTL_MIO 0x10    /* CNTL4 */
#define CNTL_RW 0x20     /* CNTL5 */
#define CNTL_INTACK 0x80 /* CNTL7 */
#define PIN_FLAG 0       /* P6.03 */
#define PIN_SENSE 1      /* P6.04 */
#define PIN_CLOCK 5      /* P9.08 */
#define PIN_RUNWAIT 29   /* P9.31 */
#define PIN_OPREQ 6      /* P7.10 */
#define PIN_WRP 9        /* P7.11 */
#define PIN_DBUSEN 32    /* P7.12 */
#define PIN_RESET 28     /* P8.18 */
#define PIN_ADREN 31     /* P8.22 */
#define PIN_OPACK 30     /* P8.23 */

#include "pins.h"
#include "signals_scn2650.h"

namespace debugger {
namespace scn2650 {

struct PinsScn2650 final : Pins {
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

private:
    void setBreakInst(uint32_t addr) const override;

    Signals *prepareCycle();
    Signals *completeCycle(Signals *signals);
    void loop();
    bool rawStep();
    uint8_t execute(const uint8_t *inst, uint8_t len, uint16_t *addr,
            uint8_t *buf, uint8_t max);

    void disassembleCycles();
};

extern struct PinsScn2650 Pins;

}  // namespace scn2650
}  // namespace debugger

#endif /* __PINS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
