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

    void setBreakInst(uint32_t addr) const override;
    void printCycles() override;

    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;

    virtual bool is_internal(uint16_t addr) const = 0;
    virtual void injectReads(const uint8_t *inst, uint_fast8_t len,
            uint_fast8_t cycles, bool discard = false) = 0;
    virtual uint16_t captureWrites(
            uint8_t *buf, uint_fast8_t len, bool discard = false) = 0;
    // Whether the next bus cycle reads the SWI vector
    bool swiVector() const;

protected:
    InstMc6805 *_inst;

    virtual Signals *currCycle(uint16_t pc = 0) const = 0;
    bool _woken = false;
    uint8_t _lastOpcode = 0;
    bool checkBreakPoint(Signals *s);

    const SignalsImpl *findBacktraceStart() override;
    void printBacktrace() override;
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
