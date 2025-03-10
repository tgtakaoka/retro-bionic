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
    _buffer.bits(32, _cc, 0x80, line + 34);
    cli.println(_buffer);
}

void RegsMc68HC08::reset() {
    // initialize EBI; EBIC:$003B = IRV($40)
    // initialize clock: PCTL:$001C = 0
    // disable COP: MORA:$001A
    // initialize SP to external space
}

void RegsMc68HC08::save() {
    RegsMc6805::save();
    const uint8_t PSHH = 0x8B;
    const uint8_t PULA = 0x86;
    // TODO
    static constexpr uint8_t STA[] = {
            0xC7, 0x10, 0x00,  // STA $1000
    };
    _pins->injectReads(&PSHH, sizeof(PSHH));
    _pins->suspend();
    _pins->injectReads(&PULA, sizeof(PULA));
    _pins->suspend();
    _pins->injectReads(STA, sizeof(STA));
    _pins->captureWrites(&_h, sizeof(_h));
    _pins->suspend();
}

void RegsMc68HC08::restore() {
    const uint8_t LDHX[] = {0x45, _h, _x};
    _pins->injectReads(LDHX, sizeof(LDHX));
    RegsMc6805::restore();
}

void RegsMc68HC08::helpRegisters() const {
    cli.println("?Reg: PC HX H X A CC");
}

constexpr const char *REGS8[] = {
        "H",  // 5
};

constexpr const char *REGS16[] = {
        "HX",  // 6
};

const Regs::RegList *RegsMc68HC08::listRegisters(uint_fast8_t n) const {
    static constexpr RegList REG_8{REGS8, 1, 5, UINT8_MAX};
    static constexpr RegList REG_16{REGS16, 1, 6, UINT16_MAX};
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
