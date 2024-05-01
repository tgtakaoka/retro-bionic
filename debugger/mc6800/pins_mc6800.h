#ifndef __PINS_MC6800_H__
#define __PINS_MC6800_H__

#include "devs.h"
#include "mems.h"
#include "pins.h"
#include "signals_mc6800.h"

#define PORT_CNTL 9  /* GPIO9 */
#define CNTL_gp 4    /* P9.04-P4.07 */
#define CNTL_gm 0xF  /* P9.04-P9.07 */
#define CNTL_vp 0    /* CNTL0-CNTL3 */
#define PIN_RW 3     /* P9.05 */
#define CNTL_VMA 0x1 /* CNTL0 */
#define CNTL_RW 0x2  /* CNTL1 */
#define PIN_IRQ 6    /* P7.10 */
#define PIN_NMI 9    /* P7.11 */
#define PIN_RESET 28 /* P8.18 */

namespace debugger {
namespace mc6800 {

struct RegsMc6800;
struct InstMc6800;

struct PinsMc6800 : Pins {
    PinsMc6800(RegsMc6800 *regs, InstMc6800 *inst, const Mems *mems, Devs *devs)
        : _regs(regs), _inst(inst), _mems(mems), _devs(devs) {}

    void reset() override;
    void idle() override;
    bool step(bool show) override;
    void run() override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;
    void printCycles() override { printCycles(nullptr); }

    void injectReads(const uint8_t *inst, uint8_t len, uint8_t cycles = 0);
    void captureWrites(uint8_t *buf, uint8_t len, uint16_t *addr = nullptr);
    virtual bool nonVmaAfteContextSave() const { return true; }

protected:
    RegsMc6800 *_regs;
    InstMc6800 *_inst;
    const Mems *const _mems;
    Devs *_devs;
    uint8_t _writes;

    void setBreakInst(uint32_t addr) const override;
    virtual Signals *rawCycle();
    virtual Signals *cycle();
    virtual void suspend(bool show);
    Signals *injectCycle(uint8_t data);
    void loop();

    static void assert_reset();
    static void negate_reset();
    static void negate_nmi();
    static void assert_nmi();
    static void negate_irq();
    static void assert_irq();

    void printCycles(const Signals *end);
    bool matchAll(Signals *begin, const Signals *end);
    const Signals *findFetch(Signals *begin, const Signals *end);
    virtual void disassembleCycles();
};

}  // namespace mc6800
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
