#include "regs_tms370cx5x.h"
#include "pins_tms370cx5x.h"

namespace debugger {
namespace tms370cx5x {

RegsTms370Cx5x::RegsTms370Cx5x(PinsTms370Cx5x *pins) : RegsTms370(pins) {}

void RegsTms370Cx5x::save() {
    RegsTms370::save();
    for (uint_fast8_t i = 0; i < sizeof(_INTx); i++)
        _INTx[i] = read_peripheral(INT1 + i);
}

void RegsTms370Cx5x::restore() {
    for (uint_fast8_t i = 0; i < sizeof(_INTx); i++)
        write_peripheral(INT1 + i, _INTx[i]);
    RegsTms370::restore();
}

}  // namespace tms370cx5x
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
