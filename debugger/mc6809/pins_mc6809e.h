#ifndef __PINS_MC6809E_H__
#define __PINS_MC6809E_H__

#define PIN_AVMA 2 /* P9.04 */
#define PIN_BUSY 0 /* P6.03 */
#define PIN_E 5    /* P9.08 */
#define PIN_Q 29   /* P9.31 */
#define PIN_LIC 32 /* P7.12 */
#define PIN_TSC 31 /* P8.22 */

#include "pins_mc6809_base.h"

namespace debugger {
namespace mc6809e {

using mc6809::InstMc6809;
using mc6809::PinsMc6809Base;
using mc6809::RegsMc6809;

struct PinsMc6809E final : PinsMc6809Base {
    PinsMc6809E();

    void resetPins() override;

protected:
    mc6809::Signals *rawCycle() const override;
    mc6809::Signals *cycle() const override;

    const mc6809::Signals *findFetch(
            mc6809::Signals *begin, const mc6809::Signals *end) override;
};

}  // namespace mc6809e
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
