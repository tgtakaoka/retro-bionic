#ifndef __PINS_MC6805_H__
#define __PINS_MC6805_H__

#include "devs.h"
#include "inst_mc6805.h"
#include "mems_mc6805.h"
#include "pins.h"
#include "regs_mc6805.h"
#include "signals_mc6805.h"

namespace debugger {
namespace mc6805 {

using mc6805::InstMc6805;

struct PinsMc6805 : Pins {
    PinsMc6805(RegsMc6805 &regs, const InstMc6805 &inst, const MemsMc6805 &mems,
            Devs &devs)
        : _regs(regs), _inst(inst), _mems(mems), _devs(devs) {}

    void idle() override;
    bool step(bool show) override;
    void run() override;
    void printCycles() override;

    void injectReads(
            const uint8_t *inst, uint8_t len, uint8_t cycles = 0) const;
    void captureWrites(
            uint8_t *buf, uint8_t len, uint16_t *addr = nullptr) const;
    void suspend() const;

protected:
    RegsMc6805 &_regs;
    const InstMc6805 &_inst;
    const MemsMc6805 &_mems;
    Devs &_devs;

    virtual Signals *currCycle() const = 0;
    virtual Signals *rawPrepareCycle() const = 0;
    virtual Signals *prepareCycle() const = 0;
    virtual Signals *completeCycle(Signals *signals) const = 0;
    Signals *cycle() const;
    Signals *cycle(uint8_t data) const;
    void loop();
    bool rawStep() const;

    void setBreakInst(uint32_t addr) const override;
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
