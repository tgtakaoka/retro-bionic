#include "regs_mc6801.h"
#include "debugger.h"
#include "pins_mc6801.h"

namespace debugger {
namespace mc6801 {

/**
 * How to determine MC6800 variants.
 *
 * MC6800/MC6801/HD6301
 *   LDX  #$55AA
 *   LDAB #100
 *   LDAA #5
 *   ADDA #5
 *   FCB  $18
 *        ; DAA  ($19) on MC6800
 *        ; ABA  ($1B) on MC6801
 *        ; XGDX ($18) on HD6301
 *        ; prefix     on MC68HC11
 *   FCB  $08
 *        ; INX on MC6800/MC6801/HD6301
 *        ; INY on MC68HC11
 *   A=$10, X=$55AB: MC6800
 *   A=110, X=$55AB: MC6801
 *   A=$55, X=$0A65: HD6301
 *   A=10,  X=$55AA: MC68HC11
 *
 * MC6800/MB8861(MB8870)
 *   LDX  #$FFFF
 *   FCB  $EC, $01
 *        ; CPX 1,X ($AC $01, 6 clcoks) on MC6800
 *        ; ADX #1  ($EC $01, 2 clocks) on MB8861
 * X=$FFFF: MC6800
 * X=$0000: MB8861
 */

SoftwareType RegsMc6801::checkSoftwareType() {
    static constexpr uint8_t DETECT_6301[] = {
            0xCE, 0xFF, 0xFF,  // LDX #$FFFF ; 1:2:3
            0xC6, 0xFF,        // LDAB #$FF  ; 1:2
            0x86, 0x01,        // LDAA #$01  ; 1:2
            0x18, 0x01,        // ABA        ; 1:N
                               // XDGX       ; 1:x
            0xB7, 0x01, 0x00,  // STAA $0100 ; 1:2:3:B
    };
    _pins->injectReads(DETECT_6301, sizeof(DETECT_6301));
    uint8_t a;
    _pins->captureWrites(&a, sizeof(a));
    _type = (a == 0) ? SW_MC6801 : SW_HD6301;
    return _type;
}

void RegsMc6801::helpRegisters() const {
    cli.println("?Reg: PC SP X A B D CC");
}

constexpr const char *REGS16[] = {
        "PC",  // 4
        "SP",  // 5
        "X",   // 6
        "D",   // 7
};

const Regs::RegList *RegsMc6801::listRegisters(uint_fast8_t n) const {
    static constexpr RegList REG_6801 = {REGS16, 4, 4, UINT16_MAX};
    if (n == 0)
        return RegsMc6800::listRegisters(n);
    return n == 1 ? &REG_6801 : nullptr;
}

bool RegsMc6801::setRegister(uint_fast8_t reg, uint32_t value) {
    if (reg == 7) {
        _d(value);
        return false;
    }
    return RegsMc6800::setRegister(reg, value);
}

}  // namespace mc6801
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
