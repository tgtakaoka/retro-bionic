#ifndef __PINS_Z8_H__
#define __PINS_Z8_H__

#define EXTERNAL_STACK 1

#define PORT_DATA 6    /* GPIO6 */
#define DATA_gp 16     /* P6.16-P6.23 */
#define DATA_gm 0xFF   /* P6.16-P6.23 */
#define DATA_vp 0      /* D0-D7 */
#define PIN_DATA0 19   /* P6.16 */
#define PIN_DATA1 18   /* P6.17 */
#define PIN_DATA2 14   /* P6.18 */
#define PIN_DATA3 15   /* P6.19 */
#define PIN_DATA4 40   /* P6.20 */
#define PIN_DATA5 41   /* P6.21 */
#define PIN_DATA6 17   /* P6.22 */
#define PIN_DATA7 16   /* P6.23 */
#define PORT_ADDR 6    /* GPIO6 */
#define ADDR_gp 16     /* P6.16-P6.31 */
#define ADDR_gm 0xFFFF /* P6.16-P6.31 */
#define ADDR_vp 0      /* A0-A15 */
#define PIN_ADDR0 19   /* P6.16 */
#define PIN_ADDR1 18   /* P6.17 */
#define PIN_ADDR2 14   /* P6.18 */
#define PIN_ADDR3 15   /* P6.19 */
#define PIN_ADDR4 40   /* P6.20 */
#define PIN_ADDR5 41   /* P6.21 */
#define PIN_ADDR6 17   /* P6.22 */
#define PIN_ADDR7 16   /* P6.23 */
#define PIN_ADDR8 22   /* P6.24 */
#define PIN_ADDR9 23   /* P6.25 */
#define PIN_ADDR10 20  /* P6.26 */
#define PIN_ADDR11 21  /* P6.27 */
#define PIN_ADDR12 38  /* P6.28 */
#define PIN_ADDR13 39  /* P6.29 */
#define PIN_ADDR14 26  /* P6.30 */
#define PIN_ADDR15 27  /* P6.31 */
#define PIN_AS 2       /* P9.04 */
#define PIN_RW 3       /* P9.05 */
#define PIN_IRQ1 4     /* P9.06 */
#define PIN_IRQ0 33    /* P9.07 */
#define PIN_TXD 0      /* P6.03 */
#define PIN_RXD 1      /* P6.04 */
#define PIN_XTAL1 5    /* P9.08 */
#define PIN_DS 29      /* P9.31 */
#define PIN_IRQ2 6     /* P7.10 */
#define PIN_RESET 28   /* P8.18 */
#define PIN_DM 30      /* P8.23 */

#include "devs.h"
#include "inst_z8.h"
#include "mems.h"
#include "pins.h"
#include "regs_z8.h"
#include "signals_z8.h"

namespace debugger {
namespace z8 {

enum IntrName : uint8_t {
    INTR_NONE = 0,
    INTR_IRQ0 = 1,
    INTR_IRQ1 = 2,
    INTR_IRQ2 = 3,
};

struct PinsZ8 : Pins {
    void reset() override;
    bool step(bool show) override;
    void run() override;
    void idle() override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;
    void printCycles() override;

    Signals *cycle(uint8_t insn);
    void execInst(const uint8_t *inst, uint8_t len);
    void captureWrites(const uint8_t *inst, uint8_t len, uint16_t *addr,
            uint8_t *buf, uint8_t max);

protected:
    PinsZ8(RegsZ8 &regs, const InstZ8 &inst, Mems &mems, Devs &devs)
        : Pins(), _regs(regs), _inst(inst), _mems(mems), _devs(devs) {}

    RegsZ8 &_regs;
    const InstZ8 &_inst;
    const Mems &_mems;
    Devs &_devs;
    uint8_t _writes;

    void setBreakInst(uint32_t addr) const override;
    virtual void xtal1_cycle() const = 0;

    Signals *prepareCycle();
    Signals *completeCycle(Signals *signals);
    Signals *cycle();
    void loop();
    virtual bool fetchVectorAfterContextSave() = 0;
    bool rawStep();
    void intrAck(Signals *frame) const;
    void execute(const uint8_t *inst, uint8_t len, uint16_t *addr, uint8_t *buf,
            uint8_t max);

    void disassembleCycles();
};

}  // namespace z8
}  // namespace debugger
#endif /* __PINS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
