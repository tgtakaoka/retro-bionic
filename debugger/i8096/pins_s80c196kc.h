#ifndef __PINS_S80C196KC_H__
#define __PINS_S80C196KC_H__

#define CNTL_gm 0x1F /* P9.04-P9.08 */
#define PIN_INST 5   /* P9.08 */
#define PIN_NMI 6    /* P7.10 */
#define PIN_HOLD 9   /* P7.11 */
#define PIN_HLDA 32  /* P7.12 */

#include "pins_i8096.h"

namespace debugger {
namespace s80c196kc {

// The QFP-80 S80C196KC, or the S80C196KB on its pins: INST and NMI as the
// N8097BH, and #HOLD and #HLDA where it has ACH5 and ACH6.
struct PinsS80C196KC final : i8096::PinsI8096 {
    explicit PinsS80C196KC(i8096::CpuType cpu) : PinsI8096(cpu) {}

    void assertInt(uint8_t name) override;
    void negateInt(uint8_t name) override;

private:
    void resetPins() override;
    uint16_t haltEntry() const override { return i8096::InstI8096::VEC_NMI; }
    bool getControl(Signals *s) const override;
    bool hasInst() const override { return true; }
    Signals *startCycle(Signals *s) override;
    Signals *completeCycle(Signals *s, bool low) override;
};

}  // namespace s80c196kc
}  // namespace debugger
#endif /* __PINS_S80C196KC_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
