#include "regs_mc6800.h"
#include "debugger.h"
#include "mems_mc6800.h"
#include "pins_mc6800_base.h"

namespace debugger {
namespace mc6800 {
namespace {
const char *const CPU[] = {
        "MC6800",    // SW_MC6800
        "MB8861",    // SW_MB8861
        "MC6801",    // SW_MC6801
        "HD6301",    // SW_HD6301
        "MC68HC11",  // SW_MC68HC11
};

//                   012345678901234567890123456789012345678901
const char line[] = "PC=xxxx SP=xxxx X=xxxx A=xx B=xx CC=HINZVC";
}  // namespace

RegsMc6800::RegsMc6800(PinsMc6800Base *pins) : _pins(pins), _buffer(line) {}

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

SoftwareType RegsMc6800::checkSoftwareType() {
    static constexpr uint8_t DETECT_8861[] = {
            0xCE, 0xFF, 0xFF,  // LDX #$FFFF ; 1:2:3
            0xEC, 0x01,        // CPX 1,X    ; 1:2:n:n:R:r
                               // ADX #1     ; 1:2:n:n
            0x01, 0x01,        // NOP        ; 1:N
            0x01, 0x01,        // NOP        ; 1:N
            0xFF, 0x01, 0x00,  // STX $0100  ; 1:2:3:n:B:b
    };
    _pins->injectReads(DETECT_8861, sizeof(DETECT_8861));
    uint8_t x[2];
    _pins->captureWrites(x, sizeof(x));
    _type = x[0] ? SW_MC6800 : SW_MB8861;
    return _type;
}

const char *RegsMc6800::cpu() const {
    return CPU[_type];
}

const char *RegsMc6800::cpuName() const {
    return cpu();
}

void RegsMc6800::print() const {
    _buffer.hex16(3, _pc);
    _buffer.hex16(11, _sp);
    _buffer.hex16(18, _x);
    _buffer.hex8(25, _a);
    _buffer.hex8(30, _b);
    _buffer.bits(36, _cc, 0x20, line + 36);
    _pins->idle();
    cli.println(_buffer);
}

const uint8_t RegsMc6800::LDS_7FFF[3] = {
        0x8E, 0x7F, 0xFF,  // LDS #$7FFF ; 1:2:3
};

const uint8_t RegsMc6800::STAA_8000[3] = {
        0xB7, 0x80, 0x00,  // STAA $8000 ; 1:2:3:n:B
};

void RegsMc6800::reset() {
    _pins->injectReads(LDS_7FFF, sizeof(LDS_7FFF));
}

void RegsMc6800::save() {
    const uint8_t SWI = 0x3F;  // 1:N:w:W:W:W:W:W:W:n:V:v
                               // 1:N:x:w:W:W:W:W:W:W:V:v (HD6301)
    _pins->injectReads(&SWI, sizeof(SWI));
    uint8_t context[7];
    _pins->captureWrites(context, sizeof(context), &_sp);
    // Capturing writes to stack in little endian order.
    _pc = le16(context) - 1;  //  offset SWI instruction.
    _x = le16(context + 2);
    _a = context[4];
    _b = context[5];
    _cc = context[6];
    // Read SWI vector
    const auto readVector = _pins->nonVmaAfteContextSave() ? 3 : 2;
    context[0] = 0;  // irrelevant data
    context[readVector - 2] = hi(_pc);
    context[readVector - 1] = lo(_pc);
    _pins->injectReads(context, readVector);
}

void RegsMc6800::restore() {
    const uint16_t sp = _sp - 7;  // offset RTI
    // clang-format off
    const uint8_t LDS_RTI[3 + 10] = {
            0x8E, hi(sp), lo(sp),  // LDS #sp ; 1:2:3
            0x3B, 0,0,             // RTI     ; 1:N:n:R:r:r:r:r:r:r
            _cc,
            _b, _a,
            hi(_x), lo(_x),
            hi(_pc), lo(_pc),
    };
    // clang-format on
    _pins->injectReads(LDS_RTI, sizeof(LDS_RTI));
}

uint16_t RegsMc6800::capture(const Signals *stack, bool step) {
    _sp = stack->addr;
    _pc = uint16(stack->next(1)->data, stack->data);
    if (!step)
        --_pc;
    _x = uint16(stack->next(3)->data, stack->next(2)->data);
    _a = stack->next(4)->data;
    _b = stack->next(5)->data;
    _cc = stack->next(6)->data;
    return _pc;
}

void RegsMc6800::helpRegisters() const {
    cli.println("?Reg: PC SP X A B CC");
}

constexpr const char *REGS8[] = {
        "A",   // 1
        "B",   // 2
        "CC",  // 3
};
constexpr const char *REGS16[] = {
        "PC",  // 4
        "SP",  // 5
        "X",   // 6
};

const Regs::RegList *RegsMc6800::listRegisters(uint8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS8, 3, 1, UINT8_MAX},
            {REGS16, 3, 4, UINT16_MAX},
    };
    return n < 2 ? &REG_LIST[n] : nullptr;
}

void RegsMc6800::setRegister(uint8_t reg, uint32_t value) {
    switch (reg) {
    case 4:
        _pc = value;
        break;
    case 5:
        _sp = value;
        break;
    case 6:
        _x = value;
        break;
    case 1:
        _a = value;
        break;
    case 2:
        _b = value;
        break;
    case 3:
        _cc = value;
        break;
    }
}

}  // namespace mc6800
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
