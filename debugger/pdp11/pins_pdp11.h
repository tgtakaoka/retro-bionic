#ifndef __PINS_PDP11_H__
#define __PINS_PDP11_H__

#include "pins.h"

namespace debugger {
namespace pdp11 {

// What the PDP-11 family's register save and restore need from a
// board.
struct PinsPdp11 : Pins {
    // Exit at the first fetch past the sequence.
    static constexpr uint32_t EXIT_END = UINT32_MAX;

    // Runs |words| words of |inst| on the CPU parked at |org|, answering
    // reads of the |stackWords| words at |stackAt| from |stack|, and
    // parks at the first fetch of |exit| or past the sequence once all
    // of it was read; |org| becomes the park.
    virtual void execInst(const uint16_t *inst, uint_fast8_t words,
            uint16_t &org, uint32_t exit = EXIT_END,
            const uint16_t *stack = nullptr, uint16_t stackAt = 0,
            uint_fast8_t stackWords = 0) = 0;
    // The same, capturing up to |max| writes into |buf| instead of
    // memory, and parking at |org| again.
    virtual void captureWrites(const uint16_t *inst, uint_fast8_t words,
            uint16_t *buf, uint_fast8_t max, uint16_t &org) = 0;

protected:
    PinsPdp11() {}
};

}  // namespace pdp11
}  // namespace debugger
#endif /* __PINS_PDP11_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
