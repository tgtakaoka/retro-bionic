#include "inst_tms320c2x.h"

namespace debugger {
namespace tms320c2x {

uint_fast8_t InstTms320C2X::words(uint16_t inst) {
    // branches, calls, BLKP and BLKD (Fxxx), long immediates (Dxxx), MACD
    // and MAC (5C/5D), BC and BNC (5E80/5F80) take the next word too
    const auto hi = inst >> 8;
    if (hi >= 0xF0 || (hi & 0xF0) == 0xD0 || hi == 0x5C || hi == 0x5D)
        return 2;
    if ((hi == 0x5E || hi == 0x5F) && (inst & 0x80))
        return 2;
    return 1;
}

bool InstTms320C2X::table(uint16_t inst) {
    // TBLR, TBLW, MACD, MAC, BLKP
    const auto hi = inst >> 8;
    return hi == 0x58 || hi == 0x59 || hi == 0x5C || hi == 0x5D || hi == 0xFC;
}

bool InstTms320C2X::repeat(uint16_t inst) {
    // RPT, RPTK
    const auto hi = inst >> 8;
    return hi == 0x4B || hi == 0xCB;
}

}  // namespace tms320c2x
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
