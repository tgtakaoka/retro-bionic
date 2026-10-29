#ifndef __PINS_Z80_BASE_H__
#define __PINS_Z80_BASE_H__

#include <Arduino.h>
#include "pins.h"

namespace debugger {
namespace z80 {

/** Waits up to |limit| Teensy cycles for |done| to return true. */
template <typename F>
inline bool waitUntil(F done, uint32_t limit) {
    const uint32_t start = ARM_DWT_CYCCNT;
    while (!done()) {
        if (ARM_DWT_CYCCNT - start >= limit)
            return false;
    }
    return true;
}

struct MemsZ80;
struct RegsZ80;

struct PinsZ80Base : Pins {
    void execInst(const uint8_t *inst, uint_fast8_t len);
    uint16_t captureWrites(const uint8_t *inst, uint_fast8_t len,
            uint8_t *buf, uint_fast8_t max);
    void setBreakInst(uint32_t addr) const override;
#ifdef TUNE_CLOCK
    uint32_t clockInput() const override { return _clockInput; }
    bool setClockInput(uint32_t hz) override;
#endif

    virtual ~PinsZ80Base();

protected:
#ifdef TUNE_CLOCK
    // The PWM clock input's rate, which K sets; 0 for a bit-banged clock.
    uint32_t _clockInput = 0;
    // Times the PWM clock input to |hz|: the rate it runs at, or 0 for one
    // the CPU can't take.
    virtual uint32_t setClockRate(uint32_t) { return 0; }
#endif
    virtual uint16_t execute(const uint8_t *inst, uint_fast8_t len,
            uint8_t *buf, uint_fast8_t max) = 0;
};

}  // namespace z80
}  // namespace debugger
#endif /* __PINS_Z80_BASE_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
