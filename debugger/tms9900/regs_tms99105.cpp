#include "regs_tms99105.h"
#include "debugger.h"
#include "inst_tms9900.h"
#include "macro_tms99110.h"
#include "pins_tms99105.h"

namespace debugger {
namespace tms99105 {
namespace {
// clang-format off
//                              1         2         3         4         5
//                    01234567890123456789012345678901234567890123456789012345678
const char line1[] = "PC=xxxx  WP=xxxx  ST=LAECVPXPM_AX1111  MACRO=(PROTOTYPING)";
// clang-format on
const char TMS99110[] = "TMS99110";

const char *const MACRO_MODE[] = {
        "BASELINE",
        "STANDARD",
        "PROTOTYPING",
        TMS99110,
};
}  // namespace

RegsTms99105::RegsTms99105(PinsTms99105 *pins, Mems *mems)
    : RegsTms9900(pins, mems),
      _macroMode(MACRO_STANDARD),
      _modeValid(false),
      _tms99110(false) {
    _buffer1.set(line1);
}

const char *RegsTms99105::cpu() const {
    return _tms99110 ? TMS99110 : "TMS99105";
}

void RegsTms99105::print() const {
    uint_fast8_t pos = 45;
    if (!_modeValid)
        _buffer1[pos++] = '(';
    pos = _buffer1.text(pos, MACRO_MODE[_macroMode]);
    if (!_modeValid)
        _buffer1[pos++] = ')';
    _buffer1[pos] = 0;
    RegsTms9900::print();
}

void RegsTms99105::reset() {
    _modeValid = true;
    _wp = _mems->read(tms9900::InstTms9900::VEC_RESET + 0);
    _pc = _mems->read(tms9900::InstTms9900::VEC_RESET + 2);
}

void RegsTms99105::restore() {
    const uint16_t RTWP[] = {
            0x0380,  // RTWP
            _pc,     // WR14: new PC
            _st,     // WR15: new ST
            _wp,     // WR13: new WP
    };
    _pins->injectReads(RTWP, length(RTWP));
}

void RegsTms99105::save() {
    uint16_t buf[3];
    _pins->injectReads(&ZERO, 1);  // inject new WP
    _pins->injectReads(&ZERO, 1);  // inject new PC
    _pins->captureWrites(buf, 3);  // capture old context
    _wp = buf[0];                  // WR13: old WP
    _pc = buf[1];                  // WR14: old PC
    _st = buf[2];                  // WR15: old ST
}

void RegsTms99105::breakPoint() {
    uint16_t buf[3];
    _pins->injectReads(&ZERO, 1);  // inject new WP
    _pins->captureWrites(buf, 1);  // WR11: capture new R11
    _pins->injectReads(&ZERO, 1);  // inject new PC
    _pins->captureWrites(buf, 3);  // capture old context
    _wp = buf[0];                  // WR13: old WP
    _pc = buf[1];                  // WR14: old PC
    _st = buf[2];                  // WR15: old ST
    _pc -= 2;                      // offset break point XOP
}

void RegsTms99105::helpRegisters() const {
    cli.println(F("?Reg: PC WP ST R0-R15 MACRO"));
}

constexpr const char *REGS3[] = {
        "MACRO",  // 20
};

const Regs::RegList *RegsTms99105::listRegisters(uint_fast8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS3, 1, 20, 3},
    };
    if (n == 1)
        return &REG_LIST[0];
    return RegsTms9900 ::listRegisters(n);
}

bool RegsTms99105::setRegister(uint_fast8_t reg, uint32_t value) {
    if (reg == 20 && _macroMode != MacroMode(value)) {
        _macroMode = MacroMode(value);
        _modeValid = false;
        cli.println();
        if (_macroMode == MACRO_TMS99110) {
            tms99110::MacroTms99110::load(mems<MemsTms99105>());
            cli.println("TMS99110 Macrostore ROM loaded");
        }
        cli.print("!!! Need Reset to change Macrostore mode");
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
