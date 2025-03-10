#include "regs_mc68hc05.h"
#include "debugger.h"
#include "pins_mc68hc05c0.h"

namespace debugger {
namespace mc68hc05 {

RegsMc68HC05::RegsMc68HC05(mc6805::PinsMc6805 *pins)
    : RegsMc6805("MC68HC05", pins) {}

void RegsMc68HC05::reset() {
    // If reset vector pointing internal memory, we can't inject instructions.
    static constexpr uint8_t DUMMY_RESET[] = {0x10, 0x00, 0x00};
    _pins->injectReads(DUMMY_RESET, sizeof(DUMMY_RESET), 0);
    // Disable COP, enable IRV and ILRV of CNFGR($19); reset value 0x63
    internal_write(0x19, 0x5B);
}

}  // namespace mc68hc05
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
