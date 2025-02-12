#include "regs_tms99105.h"
#include "debugger.h"
#include "inst_tms9900.h"
#include "pins_tms99105.h"

namespace debugger {
namespace tms99105 {
namespace {
// clang-format off
//                    01234567890123456789012345678901234567
const char line1[] = "PC=xxxx  WP=xxxx  ST=LAECVPXPM_AX1111";
// clang-format on
}  // namespace

RegsTms99105::RegsTms99105(PinsTms99105 *pins, Mems *mems)
    : RegsTms9900(pins, mems), _macro(MACRO_STANDARD), _tms99110(false) {
    _buffer1.set(line1);
}

const char *RegsTms99105::cpu() const {
    return _tms99110 ? "TMS99110" : "TMS99105";
}

void RegsTms99105::restore() {
    const uint16_t RTWP[] = {
            0x0380,  // RTWP
            _pc,     // WR14: new PC
            _st,     // WR15: new ST
            _wp,     // WR13: new WP
    };
    pins<PinsTms99105>()->injectReads(RTWP, length(RTWP));
}

const uint16_t WZERO = 0;

void RegsTms99105::save() {
    uint16_t buf[3];
    pins<PinsTms99105>()->injectReads(&WZERO, 1);  // inject new WP
    pins<PinsTms99105>()->injectReads(&WZERO, 1);  // inject new PC
    pins<PinsTms99105>()->captureWrites(buf, 3);   // capture old context
    _wp = buf[0];                                  // WR13: old WP
    _pc = buf[1];                                  // WR14: old PC
    _st = buf[2];                                  // WR15: old ST
}

void RegsTms99105::breakPoint() {
    uint16_t buf[3];
    pins<PinsTms99105>()->injectReads(&WZERO, 1);  // inject new WP
    pins<PinsTms99105>()->captureWrites(buf, 1);   // WR11: capture new R11
    pins<PinsTms99105>()->injectReads(&WZERO, 1);  // inject new PC
    pins<PinsTms99105>()->captureWrites(buf, 3);   // capture old context
    _wp = buf[0];                                  // WR13: old WP
    _pc = buf[1];                                  // WR14: old PC
    _st = buf[2];                                  // WR15: old ST
    _pc -= 2;                                      // offset break point XOP
}

void RegsTms99105::helpRegisters() const {
    cli.println(F("?Reg: PC WP ST R0-R15 MACRO"));
}

constexpr const char *REGS1[] = {
        "MACRO",  // 20
};

const Regs::RegList *RegsTms99105::listRegisters(uint_fast8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS1, 1, 20, 2},
    };
    return n == 1 ? &REG_LIST[0] : RegsTms9900::listRegisters(n);
}

bool RegsTms99105::setRegister(uint_fast8_t reg, uint32_t value) {
    if (reg == 20) {
        _macro = MacroMode(value);
        return false;
    }
    return RegsTms9900::setRegister(reg, value);
}

}  // namespace tms99105
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
