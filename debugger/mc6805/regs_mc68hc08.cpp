#include "regs_mc68hc08.h"
#include "debugger.h"
#include "mems_mc6805.h"
#include "pins_mc6805.h"

namespace debugger {
namespace mc68hc08 {
namespace {
//                   0123456789012345678901234567890123456789
const char line[] = "PC=xxxx SP=xxxx HX=xxxx A=xx CC=V__HINZC";
}  // namespace

RegsMc68HC08::RegsMc68HC08(mc6805::PinsMc6805 *pins)
    : RegsMc6805("MC68HC08", pins) {
    _buffer.set(line);
}

void RegsMc68HC08::print() const {
    _buffer.hex16(3, _pc);
    _buffer.hex16(11, _sp);
    _buffer.hex8(19, _h);
    _buffer.hex8(21, _x);
    _buffer.hex8(26, _a);
    _buffer.bits(32, _cc, 0x80, line + 32);
    cli.println(_buffer);
}

void RegsMc68HC08::reset() {
    RegsMc6805::reset();
    // Turns off all wait control; EBICS($3C) = 0
    static constexpr uint8_t CLR_EBICS[] = {
            0x3F, 0x3C,  // CLR $3C; 0:2:E:N
    };
    // Internal bus activities are not yet visible here
    _pins->injectReads(CLR_EBICS, sizeof(CLR_EBICS), 0);
    // Turns on MCLK enable: DDRC($06) |= MCLKEN($80)
    static constexpr uint8_t BSET_MCLKEN[] = {
            0x1E, 0x06,  // BSET 7,$06; 0:2:D:E:N
    };
    _pins->injectReads(BSET_MCLKEN, sizeof(BSET_MCLKEN), 0);
    // Turns on internal read visibility; EBIC($3B) |= IRV($40)
    static constexpr uint8_t BSET_IRV[] = {
            0x1C, 0x3B,  // BSET 6,$3B; 0:2:D:E:N
    };
    _pins->injectReads(BSET_IRV, sizeof(BSET_IRV), 0);
}

void RegsMc68HC08::saveH() {
    static constexpr uint8_t PSHH[] = {
            0x8B, 0x9D,  // PSHH; 0:N:W
    };
    _pins->injectReads(PSHH, sizeof(PSHH), 0);
    _pins->captureWrites(&_h, sizeof(_h));
    const uint8_t PULH[] = {
            0x8A, 0x9D, _h,  // PULH; 0:N:R
    };
    _pins->injectReads(PULH, sizeof(PULH), 0);
}

void RegsMc68HC08::save() {
    RegsMc6805::save();
    saveH();
}

void RegsMc68HC08::restore() {
    const uint8_t LDSP_HX[] = {
            0x45, hi(_sp - 4), lo(_sp - 4),  // LDHX #_sp-4; 0:2:3:N
            0x94, 0x45,                      // TXS        ; 0:N:N
            0x45, _h, _x,                    // LDHX #_h_x ; 0:2:3:N
    };
    _pins->injectReads(LDSP_HX, sizeof(LDSP_HX), 0);
    RegsMc6805::restore();
}

void RegsMc68HC08::captureExtra(uint16_t pc) {
    RegsMc6805::captureExtra(pc);
    saveH();
}

void RegsMc68HC08::helpRegisters() const {
    cli.println("?Reg: PC SP HX H X A CC");
}

constexpr const char *REGS8[] = {
        "H",  // 5
};

constexpr const char *REGS16[] = {
        "SP",  // 6
        "HX",  // 7
};

const Regs::RegList *RegsMc68HC08::listRegisters(uint_fast8_t n) const {
    static constexpr RegList REG_8{REGS8, 1, 5, UINT8_MAX};
    static constexpr RegList REG_16{REGS16, 2, 6, UINT16_MAX};
    if (n == 2)
        return &REG_8;
    if (n == 3)
        return &REG_16;
    return RegsMc6805::listRegisters(n);
}

bool RegsMc68HC08::setRegister(uint_fast8_t reg, uint32_t value) {
    switch (reg) {
    case 5:
        _h = value;
        break;
    case 6:
        _sp = value;
        break;
    case 7:
        _h = hi(value);
        _x = lo(value);
        break;
    default:
        return RegsMc6805::setRegister(reg, value);
    }
    return false;
}

}  // namespace mc68hc08
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
