#ifndef __PINS_P8095BH_H__
#define __PINS_P8095BH_H__

#define PORT_CNTL 9 /* GPIO9 */
#define CNTL_gp 4   /* P9.04-P9.07 */
#define CNTL_gm 0xF /* P9.04-P9.07 */
#define CNTL_vp 0   /* CNTL0-CNTL3 */
#define PIN_PWM 5   /* P9.08 */
#define PIN_ACH4 6  /* P7.10 */
#define PIN_ACH5 9  /* P7.11 */
#define PIN_ACH6 32 /* P7.12 */

#include "pins_i8096.h"

namespace debugger {
namespace p8095bh {

struct PinsP8095BH final : i8096::PinsI8096 {
private:
    void resetPins() override;
    bool getControl(Signals *s) const override;
};

}  // namespace p8095bh
}  // namespace debugger
#endif /* __PINS_P8095BH_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
