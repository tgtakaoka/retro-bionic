#ifndef __PINS_MC68000_H__
#define __PINS_MC68000_H__

#include "pins.h"

namespace debugger {
namespace mc68000 {

// What the register save and restore need from a board's bus: injected
// sequences on the parked CPU. Each board class implements them.
struct PinsMc68000 : Pins {
    // Where an injected sequence leaves off: back at its own origin, or --
    // given as a plain address -- the target of the jump it ends with.
    static constexpr uint32_t EXIT_ORG = UINT32_MAX;

    // Run |inst| on the parked CPU: its reads inside the window are fed
    // from |inst| by address, other program reads get NOPs, data reads
    // outside it come from memory, and every write is captured, never
    // stored. The first |max| bytes written go to |buf| and their
    // addresses to |addrs|; the count captured is returned. |org| is where
    // the CPU is parked, and is advanced to where the sequence leaves off.
    // Instructions are words, so |org| and |len| are even, and every
    // sequence ends in a taken branch: the prefetch reaches the exit before
    // the last instructions execute.
    virtual void execInst(const uint8_t *inst, uint_fast8_t len,
            uint32_t &org, uint32_t exit) = 0;
    virtual uint_fast8_t captureWrites(const uint8_t *inst, uint_fast8_t len,
            uint8_t *buf, uint32_t *addrs, uint_fast8_t max, uint32_t &org,
            uint32_t exit) = 0;
    // Complete the vector read the CPU is parked in with |handler|, and
    // park it again at the handler's first fetch.
    virtual bool answerVector(uint32_t vecAddr, uint32_t handler) = 0;
    // Whether the last sequence ended anywhere but its exit.
    bool cutShort() const { return _cutShort; }

protected:
    bool _cutShort = false;
};

}  // namespace mc68000
}  // namespace debugger
#endif /* __PINS_MC68000_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
