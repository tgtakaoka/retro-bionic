#include "inst_cp1610.h"

namespace debugger {
namespace cp1600 {

uint_fast8_t InstCp1610::busCycles(uint16_t inst, bool sdbd) {
    const auto opc = inst & 0x3FF;
    if (opc == J)
        return 2;  // 2 address words
    if (opc < 0x200)
        return 0;  // register-register, shift and control
    if (opc < 0x240)
        return 1;  // displacement
    const auto mode = (opc >> 3) & 7;
    if (mode == 0)
        return 2;  // ADAR then DTB or DW/DWS
    const auto write = (opc & 0x1C0) == 0x040;
    return (write || !sdbd) ? 1 : 2;
}

}  // namespace cp1600
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
