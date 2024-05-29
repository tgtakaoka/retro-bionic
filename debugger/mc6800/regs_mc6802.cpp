#include "regs_mc6802.h"
#include "mems_mc6802.h"
#include "pins_mc6802.h"

namespace debugger {
namespace mc6802 {

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

static const char MC6802[] = "MC6802";
static const char MC6808[] = "MC6808";
static const char MB8870[] = "MB8870";

const char *RegsMc6802::cpuName() const {
    if (_type == SW_MC6800)
        return Memory.is_internal(0) ? MC6802 : MC6808;
    if (_type == SW_MB8861)
        return MB8870;
    return RegsMc6800::cpuName();
}

uint8_t RegsMc6802::internal_read(uint16_t addr) const {
    uint8_t LDAA[3];
    LDAA[0] = 0xB6;  // LDAA addr  ; 1:2:3:A
    LDAA[1] = hi(addr);
    LDAA[2] = lo(addr);
    _pins.injectReads(LDAA, sizeof(LDAA), 4);
    _pins.injectReads(STAA_8000, sizeof(STAA_8000));
    uint8_t data;
    _pins.captureWrites(&data, sizeof(data));
    return data;
}

void RegsMc6802::internal_write(uint16_t addr, uint8_t data) const {
    uint8_t LDAA_STAA[2 + 3];
    LDAA_STAA[0] = 0x86;  // LDAA #val ; 1:2
    LDAA_STAA[1] = data;
    LDAA_STAA[2] = 0xB7;  // STAA addr ; 1:2:3:n:B
    LDAA_STAA[3] = hi(addr);
    LDAA_STAA[4] = lo(addr);
    _pins.injectReads(LDAA_STAA, sizeof(LDAA_STAA), 7);
}

}  // namespace mc6802
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
