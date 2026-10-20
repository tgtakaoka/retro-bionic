#ifndef __PINS_MC9S12D_H__
#define __PINS_MC9S12D_H__

#include "pins_mc68hc12.h"

// The MC9S12D board puts these on pins the MC68HC12B board uses for others.
#define PIN_XCLKS PIN_DBE     /* P40: XCLKS while #RESET is low */
#define PIN_NOACC PIN_DBE     /* P40: NOACC after it */
#define PIN_ROMCTL PIN_DIVBYP /* P45 */
#define PIN_XADDR14 PIN_PS2   /* P32 */
#define PIN_XADDR15 PIN_PS3   /* P33 */
#define PIN_XADDR16 PIN_PS4   /* P34 */
#define PIN_XADDR17 PIN_PS5   /* P35 */
#define PIN_XADDR18 PIN_PS6   /* P36 */
#define PIN_XADDR19 PIN_PS7   /* P37 */

namespace debugger {
namespace mc68hc12 {

struct Mc9s12dInit;

/**
 * The MC9S12DG128B (HCS12) on the MC68HC12B's bus pins. Without #DBE, a
 * read is the debugger's to answer when NOACC doesn't mark it free and its
 * address is outside; internal accesses show on the bus (IVIS).
 */
struct PinsMc9s12d final : PinsMc68hc12 {
    PinsMc9s12d(Mc9s12dInit &init);

private:
    // NOACC shows the free cycles once configSystem() set NOACCE.
    bool _noacc = false;
    // Cycles in a row without E: free cycles may not show it, STOP holds it.
    uint16_t _noE = 0;

    void resetPins() override;
    void systemConfigured() override { _noacc = true; }
    Signals *prepareCycle() override;
    Signals *completeCycle(Signals *s) override;
    void replay(Signals *begin, uint_fast8_t cycles) override;
};

}  // namespace mc68hc12
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
