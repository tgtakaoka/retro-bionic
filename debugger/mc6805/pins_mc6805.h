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
    bool step(bool show) override;
    void run() override;
    void setBreakInst(uint32_t addr) const override;
    void printCycles() override;

    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;

    virtual bool is_internal(uint16_t addr) const = 0;
    void injectReads(
            const uint8_t *inst, uint8_t len, uint8_t cycles = 0) const;
    void captureWrites(
            uint8_t *buf, uint8_t len, uint16_t *addr = nullptr) const;
    void suspend() const;

protected:
    PinsMc6805(const InstMc6805 &inst) : _inst(inst) {}
    const InstMc6805 &_inst;

    virtual Signals *currCycle() const = 0;
    virtual Signals *rawPrepareCycle() const = 0;
    virtual Signals *prepareCycle() const = 0;
    virtual Signals *completeCycle(Signals *signals) const = 0;
    Signals *cycle() const;
    Signals *inject(uint8_t data) const;
    void loop();
    bool rawStep() const;

    void disassembleCycles() const;
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
