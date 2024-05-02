#include "signals_mc6809e.h"
#include "debugger.h"
#include "pins_mc6809e.h"

namespace debugger {
namespace mc6809e {

void Signals::getControl() {
    lic() = digitalReadFast(PIN_LIC);
    vma() = 1;
}

}  // namespace mc6809e
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
