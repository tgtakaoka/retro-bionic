#ifndef __PINS_MC6805_H__
#define __PINS_MC6805_H__

#include "inst_mc6805.h"
#include "pins.h"
#include "signals_mc6805.h"

#define PIN_IRQ 6 /* P7.10 */

namespace debugger {
namespace mc6805 {

using mc6805::InstMc6805;

struct PinsMc6805 : Pins {
    virtual ~PinsMc6805() { delete _inst; }

    void resetPins() override;
    bool step(bool show) override;
    void run() override;
    void setBreakInst(uint32_t addr) const override;
    void printCycles() override;

    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;

    virtual bool is_internal(uint16_t addr) const = 0;
    void injectReads(const uint8_t *inst, uint_fast8_t len, uint_fast8_t cycles,
            bool discard = false);
    uint16_t captureWrites(
            uint8_t *buf, uint_fast8_t len, bool discard = false);
    Signals *inject(uint8_t data);

protected:
    InstMc6805 *_inst;

    virtual void resetCpu() = 0;
    virtual Signals *currCycle(uint16_t pc = 0) const = 0;
    virtual Signals *rawPrepareCycle() = 0;
    virtual Signals *prepareCycle() = 0;
    virtual Signals *completeCycle(Signals *signals) = 0;
    virtual bool rawStep();
    Signals *cycle();
    Signals *suspend(Signals *s);
    void loop();

    void disassembleCycles();
};

}  // namespace mc6805
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
