#ifndef __PINS_D8087_H__
#define __PINS_D8087_H__

#include "pins_i8086.h"
#include "pwm_clock.h"

namespace debugger {
namespace i8086 {

// The D8087 board: a P8086 and an 8087 in maximum mode. The NMOS 8087
// needs a running clock, so CLK free-runs from FlexPWM3 and the debugger
// holds each bus cycle in wait states with READY low.
struct PinsD8087 final : PinsI8086 {
    PinsD8087();

#ifdef TUNE_CLOCK
    uint32_t clockInput() const override { return _clockInput; }
    bool setClockInput(uint32_t hz) override;
#endif

private:
    PwmClock<PIN_CLK> _clock;
#ifdef TUNE_CLOCK
    // CLK's rate, which K sets.
    uint32_t _clockInput;
#endif

    void resetPins() override;
    Signals *prepareCycle() override;
    Signals *resumeCycle(uint32_t addr) override;
    Signals *completeCycle(Signals *s) override;
};

}  // namespace i8086
}  // namespace debugger
#endif /* __PINS_D8087_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
