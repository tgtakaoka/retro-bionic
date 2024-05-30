#ifndef __PINS_NSC800_H__
#define __PINS_NSC800_H__

#include "pins_z80.h"

namespace debugger {
namespace nsc800 {

using z80::PinsZ80;
using z80::RegsZ80;
using z80::Signals;

struct PinsNsc800 final : PinsZ80 {
    PinsNsc800(RegsZ80 &regs) : PinsZ80(regs) {}

    void reset() override;
    void idle() override;
    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;

protected:
    z80::Signals *prepareCycle() const override;
    z80::Signals *completeCycle(z80::Signals *signals) const override;
    z80::Signals *prepareWait() const override;
};

extern struct PinsNsc800 Pins;

}  // namespace nsc800
}  // namespace debugger
#endif /* __PINS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
