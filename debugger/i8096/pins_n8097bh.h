#ifndef __PINS_N8097BH_H__
#define __PINS_N8097BH_H__

#define PORT_CNTL 9  /* GPIO9 */
#define CNTL_gp 4    /* P9.04-P9.08 */
#define CNTL_gm 0x1F /* P9.04-P9.08 */
#define CNTL_vp 0    /* CNTL0-CNTL4 */
#define PIN_INST 5   /* P9.08 */
#define PIN_NMI 6    /* P7.10 */

#include "pins_i8096.h"

namespace debugger {
namespace n8097bh {

// The PLCC-68 N8097BH has INST and NMI where the P8095BH has PWM and ACH4.
struct PinsN8097BH final : i8096::PinsI8096 {
private:
    void resetPins() override;
    bool getControl(Signals *s) const override;
};

}  // namespace n8097bh
}  // namespace debugger
#endif /* __PINS_N8097BH_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
