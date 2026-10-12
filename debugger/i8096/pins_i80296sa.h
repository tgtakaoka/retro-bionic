#ifndef __PINS_I80296SA_H__
#define __PINS_I80296SA_H__

#define CNTL_gm 0x1F /* P9.04-P9.08 */
#define PIN_INST 5   /* P9.08 */
#define PIN_NMI 6    /* P7.10 */
#define PIN_HOLD 9   /* P7.11 */
#define PIN_HLDA 32  /* P7.12 */

#include "pins_i8096.h"

namespace debugger {
namespace s80296sa {

// The QFP-100 S80296SA, or the S80C196NP on its pins. A16-A19, CLKOUT and
// #CS0-#CS2 sit where the P8095BH has HSO.0-3 and HSI.0-3; both run in
// 64-Kbyte mode, where A15:0 alone tell the debugger's addresses.
struct PinsI80296SA final : i8096::PinsI8096 {
    explicit PinsI80296SA(i8096::CpuType cpu) : PinsI8096(cpu) {}

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

}  // namespace s80296sa
}  // namespace debugger
#endif /* __PINS_I80296SA_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
