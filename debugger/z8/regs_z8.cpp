#include "regs_z8.h"
#include "debugger.h"
#include "pins_z8.h"

namespace debugger {
namespace z8 {
namespace {
// clang-format off
//                    0123456789012345678901234567890123456789012345678901234
const char line2[] = " R0=xx  R1=xx  R2=xx  R3=xx  R4=xx  R5=xx  R6=xx  R7=xx";
const char line3[] = " R8=xx  R9=xx R10=xx R11=xx R12=xx R13=xx R14=xx R15=xx";
// clang-format on
}  // namespace

RegsZ8::RegsZ8(PinsZ8 *pins) : _pins(pins), _buffer2(line2), _buffer3(line3) {}

void RegsZ8::print() const {
    for (auto i = 0; i < 8; ++i)
        _buffer2.hex8(4 + i * 7, _r[i]);
    cli.println(_buffer2);
    for (auto i = 0; i < 8; ++i)
        _buffer3.hex8(4 + i * 7, _r[i + 8]);
    cli.println(_buffer3);
}

void RegsZ8::save() {
    _pc = _pins->cycle(InstZ8::NOP)->addr;
    save_all_r();
    save_sfrs();
}

void RegsZ8::save_all_r() {
    for (auto num = 0; num < 16; num++)
        _r[num] = save_r(num);
}

void RegsZ8::restore() {
    restore_sfrs();
    for (auto num = 0; num < 16; num++) {
        restore_r(num, _r[num]);
    }
    const uint8_t JP[] = {
            0x8D, hi(_pc), lo(_pc),  // JP _pc
    };
    _pins->execInst(JP, sizeof(JP));
}

constexpr const char *const RegsZ8::REGS8[18] = {
        "R0",     // 1
        "R1",     // 2
        "R2",     // 3
        "R3",     // 4
        "R4",     // 5
        "R5",     // 6
        "R6",     // 7
        "R7",     // 8
        "R8",     // 9
        "R9",     // 10
        "R10",    // 11
        "R11",    // 12
        "R12",    // 13
        "R13",    // 14
        "R14",    // 15
        "R15",    // 16
        "FLAGS",  // 17
        "RP",     // 18
};

constexpr const char *const RegsZ8::REGS16[10] = {
        "RR0",   // 19
        "RR2",   // 20
        "RR4",   // 21
        "RR6",   // 22
        "RR8",   // 23
        "RR10",  // 24
        "RR12",  // 25
        "RR14",  // 26
        "PC",    // 27
        "SP",    // 28
};

const Regs::RegList *RegsZ8::listRegisters(uint_fast8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS8, 18, 1, UINT8_MAX},
            {REGS16, 10, 19, UINT16_MAX},
    };
    return n < 2 ? &REG_LIST[n] : nullptr;
}

bool RegsZ8::setRegister(uint_fast8_t reg, uint32_t value) {
    switch (reg) {
    case 27:
        _pc = value;
        return true;
    case 28:
        set_sp(value);
        break;
    case 18:
        set_rp(value);
        break;
    case 17:
        set_flags(value);
        break;
    default:
        if (reg >= 19) {
            set_rr((reg - 19U) * 2, value);
        } else {
            set_r(reg - 1U, value);
        }
        break;
    }
    return false;
}

}  // namespace z8
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
