#ifndef __PINS_MC6809_H__
#define __PINS_MC6809_H__

#define PIN_XTAL 2  /* P9.04 */
#define PIN_Q 0     /* P6.03 */
#define PIN_EXTAL 5 /* P9.08 */
#define PIN_E 29    /* P9.31 */
#define PIN_MRDY 32 /* P7.12 */
#define PIN_BREQ 31 /* P8.22 */

#include "pins_mc6809_base.h"

namespace debugger {
namespace mc6809 {

struct PinsMc6809 final : PinsMc6809Base {
    PinsMc6809(RegsMc6809 &regs, InstMc6809 &inst, const Mems &mems)
        : PinsMc6809Base(regs, inst, mems) {}

    void resetPins() override;

protected:
    Signals *rawCycle() const override;
    Signals *cycle() const override;
};

}  // namespace mc6809
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
