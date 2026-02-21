#ifndef __PINS_MC6800_BASE_H__
#define __PINS_MC6800_BASE_H__

#include "inst_mc6800.h"
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
struct MemsMc6800;

struct PinsMc6800Base : Pins {
    ~PinsMc6800Base() { delete _inst; }

    void idle() override;
    bool step(bool show) override;
    void run() override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;
    void setBreakInst(uint32_t addr) const override;
    void printCycles() override { printCycles(nullptr); }

    void injectReads(const uint8_t *inst, uint8_t len, uint8_t cycles = 0);
    void captureWrites(uint8_t *buf, uint8_t len, uint16_t *addr = nullptr);
    virtual bool nonVmaAfteContextSave() const { return true; }

protected:
    InstMc6800 *_inst;
    uint8_t _writes;

    virtual Signals *rawCycle() = 0;
    virtual Signals *cycle() = 0;
    virtual void suspend(bool show);
    Signals *injectCycle(uint8_t data);
    void loop();

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
