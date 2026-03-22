#include "regs_z280.h"
#include "debugger.h"
#include "pins_z280.h"

namespace debugger {
namespace z280 {
namespace {
// clang-format off
//                              1         2         3         4         5         6
//                    01234567890123456789012345678901234567890123456789012345678901
const char line1[] = "PC=xxxx SP=xxxx  BC=xxxx DE=xxxx HL=xxxx A=xx F=SZ1H1VNC  I=xx";
const char line2[] = "IX=xxxx IY=xxxx (BC=xxxx DE=xxxx HL=xxxx A=xx F=SZ1H1VNC) R=xx";
// clang-format on
}  // namespace

RegsZ280::RegsZ280(PinsZ280 *pins)
    : _pins(pins), _buffer1(line1), _buffer2(line2) {}

void RegsZ280::print() const {
    auto main = _buffer1;
    main.hex16(3, _pc);
    main.hex16(11, _sp);
    main.hex16(20, _main.bc());
    main.hex16(28, _main.de());
    main.hex16(36, _main.hl());
    main.hex8(43, _main.a);
    main.bits(48, _main.f, 0x80, main + 48);
    main.hex8(60, _i);
    cli.println(main);
    _pins->idle();
    auto alt = _buffer2;
    alt.hex16(3, _ix);
    alt.hex16(11, _iy);
    alt.hex16(20, _alt.bc());
    alt.hex16(28, _alt.de());
    alt.hex16(36, _alt.hl());
    alt.hex8(43, _alt.a);
    alt.bits(48, _alt.f, 0x80, alt + 48);
    main.hex8(60, _r);
    cli.println(alt);
    _pins->idle();
}

void RegsZ280::reset() {
    // disable refresh
    // clang-format off
    static constexpr uint8_t NO_REFRESH[] = {
        // Refresh Rate Register at 0xFF**E8H.
        0x0E, 0x08,             // LD C, 08H ; I/O Page register
        0x21, 0xFF, 0x00,       // LD HL, HIGH16 REFRESH_RATE
        0xED, 0x6E,   // LDCTL (C), HL ; Set I/O Page register to 0xFF
        0xAF,         // XOR A
        0xD3, 0xE8,   // OUT (LOW REFRESH_RATE), A ; Disable refresh
        0x18, 0xF4,   // JR` ORG_RESET        
    };
    // clang-format on
    _pins->injectRead();
}

void RegsZ280::exchangeRegs() const {
#if 0
    static constexpr uint8_t EXCHANGE[] = {
            0x08,  // EX AF, AF'
            0xD9,  // EXX
    };
#endif
}

void RegsZ280::saveRegs(reg &regs) const {
#if 0
    static constexpr uint8_t PUSH_ALL[] = {
            0xF5,  // PUSH AF
            0xC5,  // PUSH BC
            0xD5,  // PUSH DE
            0xE5,  // PUSH HL
    };
#endif
}

void RegsZ280::restoreRegs(const reg &regs) const {
#if 0
    const uint8_t POP_ALL[] = {
            0xF1, regs.f, regs.a,  // POP AF
            0xC1, regs.c, regs.b,  // POP BC
            0xD1, regs.e, regs.d,  // POP DE
            0xE1, regs.l, regs.h,  // POP HL
    };
#endif
}

void RegsZ280::save() {
#if 0
    static constexpr uint8_t PUSH_PC[] = {
            0xC7,  // RST 0
    };
    uint8_t buffer[5];
    _sp += 1;
    _pc = be16(buffer) - 1;  // offser RST instruction
    saveRegs(_main);
    exchangeRegs();
    saveRegs(_alt);
    exchangeRegs();
    static constexpr uint8_t SAVE_OTHERS[] = {
            0xDD, 0xE5,  // PUSH IX
            0xFD, 0xE5,  // PUSH IY
            0xED, 0x57,  // LD A, I
            0x77,        // LD (HL), A
    };
    _ix = be16(buffer + 0);
    _iy = be16(buffer + 2);
    _i = buffer[4];
#endif
}

void RegsZ280::restore() {
#if 0
    const uint8_t LD_OTHERS[] = {
            0x3E, _i,                      // LD A, _i
            0xED, 0x47,                    // LD A, I
            0xFD, 0xE1, lo(_iy), hi(_iy),  // POP IY, _iy
            0xDD, 0xE1, lo(_ix), hi(_ix),  // POP IX, _ix
    };
    restoreRegs(_main);
    exchangeRegs();
    restoreRegs(_alt);
    exchangeRegs();
    const uint8_t LD_ALL[] = {
            0x31, lo(_sp), hi(_sp),  // LD SP, _sp
            0xC3, lo(_pc), hi(_pc),  // JP _pc
    };
#endif
}

void RegsZ280::helpRegisters() const {
    cli.println("?Reg: PC SP IX IY BC DE HL A F B C D E H L I R EX EXX");
}

constexpr const char *REGS8[] = {
        "F",  // 1
        "A",  // 2
        "B",  // 3
        "C",  // 4
        "D",  // 5
        "E",  // 6
        "H",  // 7
        "L",  // 8
        "I",  // 9
        "R",  // 10
};
constexpr const char *REGS16[] = {
        "PC",  // 11
        "SP",  // 12
        "BC",  // 13
        "DE",  // 14
        "HL",  // 15
        "IX",  // 16
        "IY",  // 17
};
constexpr const char *EXCHANGE[] = {
        "EX",   // 18
        "EXX",  // 19
};

const Regs::RegList *RegsZ280::listRegisters(uint_fast8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS8, 9, 1, UINT8_MAX},
            {REGS16, 7, 11, UINT16_MAX},
            {EXCHANGE, 2, 18, 1},
    };
    return n < 3 ? &REG_LIST[n] : nullptr;
}

bool RegsZ280::setRegister(uint_fast8_t reg, uint32_t value) {
    switch (reg) {
    case 11:
        _pc = value;
        return true;
    case 12:
        _sp = value;
        break;
    case 13:
        _main.setbc(value);
        break;
    case 14:
        _main.setde(value);
        break;
    case 15:
        _main.sethl(value);
        break;
    case 16:
        _ix = value;
        break;
    case 17:
        _iy = value;
        break;
    case 1:
        _main.f = value;
        break;
    case 2:
        _main.a = value;
        break;
    case 3:
        _main.b = value;
        break;
    case 4:
        _main.c = value;
        break;
    case 5:
        _main.d = value;
        break;
    case 6:
        _main.e = value;
        break;
    case 7:
        _main.h = value;
        break;
    case 8:
        _main.l = value;
        break;
    case 9:
        _i = value;
        break;
    case 10:
        _r = value;
        break;
    case 18:
        swap8(_main.a, _alt.a);
        swap8(_main.f, _alt.f);
        break;
    case 19:
        swap8(_main.b, _alt.b);
        swap8(_main.c, _alt.c);
        swap8(_main.d, _alt.d);
        swap8(_main.e, _alt.e);
        swap8(_main.h, _alt.h);
        swap8(_main.l, _alt.l);
        break;
    }
    return false;
}

}  // namespace z280
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
