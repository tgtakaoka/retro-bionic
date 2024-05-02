#ifndef __PINS_MC6800_H__
#define __PINS_MC6800_H__

#include "devs.h"
#include "mems.h"
#include "pins.h"
#include "signals_mc6800.h"

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

    virtual void assertNmi() const;
    virtual void negateNmi() const;
    virtual Signals *rawCycle();
    virtual Signals *cycle();
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
