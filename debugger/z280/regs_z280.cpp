#include "regs_z280.h"
#include "debugger.h"
#include "pins_z280.h"

namespace debugger {
namespace z280 {
namespace {
// clang-format off
//                              1         2         3         4         5         6         7
//                    0123456789012345678901234567890123456789012345678901234567890123456789
const char line1[] = "PC=xxxx SP=xxxx  BC=xxxx DE=xxxx HL=xxxx A=xx F=SZ1H1VNC  I=xx";
const char line2[] = "IX=xxxx IY=xxxx (BC=xxxx DE=xxxx HL=xxxx A=xx F=SZ1H1VNC) R=xx USP=xxxx";
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
    alt.hex8(60, _r);
    alt.hex16(67, _usp);
    cli.println(alt);
    _pins->idle();
}

void RegsZ280::reset() {
    // Disable refresh (Refresh Rate Register at 0xFF**E8H). Injected
    // once after reset rather than patched into memory, ending with a
    // jump back to the reset origin so the program still starts there.
    // The I/O Page register must go back to 0: it supplies A16-A23 on
    // every later I/O transaction, and pages FEH/FFH are decoded
    // on-chip, which would leave the peripherals unreachable.
    // clang-format off
    static constexpr uint8_t NO_REFRESH[] = {
        0x0E, 0x08,        // LD C, 08H       ; I/O Page register
        0x21, 0xFF, 0x00,  // LD HL, 00FFH
        0xED, 0x6E,        // LDCTL (C), HL   ; Set I/O Page register to 0xFF
        0xAF,              // XOR A
        0xD3, 0xE8,        // OUT (0E8H), A   ; Disable refresh
        0x21, 0x00, 0x00,  // LD HL, 0000H
        0xED, 0x6E,        // LDCTL (C), HL   ; Restore I/O Page register
        0xC3, 0x00, 0x00,  // JP 0000H        ; back to the reset origin
    };
    // clang-format on
    _pins->execInst(NO_REFRESH, sizeof(NO_REFRESH));
}

void RegsZ280::exchangeRegs() const {
    // clang-format off
    static constexpr uint8_t EXCHANGE[] = {
        0x08,  // EX AF, AF'
        0xD9,  // EXX
    };
    // clang-format on
    _pins->execInst(EXCHANGE, sizeof(EXCHANGE));
}

void RegsZ280::saveRegs(reg &regs) const {
    // PUSH is a single 16-bit write, landing as [low, high].
    // clang-format off
    static constexpr uint8_t PUSH_ALL[] = {
        0xF5,  // PUSH AF
        0xC5,  // PUSH BC
        0xD5,  // PUSH DE
        0xE5,  // PUSH HL
    };
    // clang-format on
    uint8_t buffer[8];
    _pins->captureWrites(PUSH_ALL, sizeof(PUSH_ALL), buffer, sizeof(buffer));
    regs.f = buffer[0];
    regs.a = buffer[1];
    regs.c = buffer[2];
    regs.b = buffer[3];
    regs.e = buffer[4];
    regs.d = buffer[5];
    regs.l = buffer[6];
    regs.h = buffer[7];
}

void RegsZ280::restoreRegs(const reg &regs) const {
    // F has no immediate load, so AF goes through a faked POP whose
    // operand arrives as a separate stack read, answered with the next
    // whole word here. That only lines up if the POP starts on an even
    // offset, hence the NOP.
    // clang-format off
    const uint8_t RESTORE[] = {
        0x01, regs.c, regs.b,  // LD BC, regs.bc()
        0x11, regs.e, regs.d,  // LD DE, regs.de()
        0x21, regs.l, regs.h,  // LD HL, regs.hl()
        0x00,                  // NOP   ; pad POP AF to an even offset
        0xF1, 0x00,            // POP AF
        regs.f, regs.a,        //       ; answers POP AF's stack read
    };
    // clang-format on
    _pins->execInst(RESTORE, sizeof(RESTORE));
}

void RegsZ280::save() {
    // RST 0 pushes the current PC as one word -- the resume point --
    // and jumps to 0x0000.
    // clang-format off
    static constexpr uint8_t PUSH_PC[] = {
        0xC7,  // RST 0
    };
    // clang-format on
    uint8_t buffer[8];
    const auto addr = _pins->captureWrites(PUSH_PC, sizeof(PUSH_PC), buffer, 2);
    _sp = addr + 2;
    _pc = le16(buffer) - 1;  // offset the injected RST opcode
    saveRegs(_main);
    exchangeRegs();
    saveRegs(_alt);
    exchangeRegs();
    // LDCTL and LD A,I/R have no bus transfer of their own, so the
    // value is read back through a captured LD (HL),A.
    // clang-format off
    static constexpr uint8_t SAVE_OTHERS[] = {
        0xDD, 0xE5,  // PUSH IX
        0xFD, 0xE5,  // PUSH IY
        0xED, 0x87,  // LDCTL HL, USP
        0xE5,        // PUSH HL
        0xED, 0x57,  // LD A, I
        0x77,        // LD (HL), A   ; captured, reads back I
        0xED, 0x5F,  // LD A, R
        0x77,        // LD (HL), A   ; captured, reads back R
    };
    // clang-format on
    _pins->captureWrites(
            SAVE_OTHERS, sizeof(SAVE_OTHERS), buffer, sizeof(buffer));
    _ix = le16(buffer + 0);
    _iy = le16(buffer + 2);
    _usp = le16(buffer + 4);
    _i = buffer[6];
    _r = buffer[7];
}

void RegsZ280::restore() {
    // POP IY/IX have 2-byte opcodes, already word aligned, so unlike
    // the 1-byte POPs they need no padding.
    // clang-format off
    const uint8_t LD_OTHERS[] = {
        0x3E, _i,            // LD A, _i
        0xED, 0x47,          // LD I, A
        0x3E, _r,            // LD A, _r
        0xED, 0x4F,          // LD R, A
        0xE1, 0x00,          // POP HL       ; NOP pads it to a whole word
        lo(_usp), hi(_usp),  //              ; answers POP HL's stack read
        0xED, 0x8F,          // LDCTL USP, HL
        0xFD, 0xE1,          // POP IY
        lo(_iy), hi(_iy),    //              ; answers POP IY's stack read
        0xDD, 0xE1,          // POP IX
        lo(_ix), hi(_ix),    //              ; answers POP IX's stack read
    };
    // clang-format on
    _pins->execInst(LD_OTHERS, sizeof(LD_OTHERS));
    exchangeRegs();
    restoreRegs(_alt);
    exchangeRegs();
    restoreRegs(_main);
    // SP last: every faked POP above bumps the real SP regardless.
    // clang-format off
    const uint8_t LD_ALL[] = {
        0x31, lo(_sp), hi(_sp),  // LD SP, _sp
        0xC3, lo(_pc), hi(_pc),  // JP _pc
    };
    // clang-format on
    _pins->execInst(LD_ALL, sizeof(LD_ALL));
}

void RegsZ280::helpRegisters() const {
    cli.println("?Reg: PC SP USP IX IY BC DE HL A F B C D E H L I R EX EXX");
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
        "PC",   // 11
        "SP",   // 12
        "USP",  // 13
        "BC",   // 14
        "DE",   // 15
        "HL",   // 16
        "IX",   // 17
        "IY",   // 18
};
constexpr const char *EXCHANGE[] = {
        "EX",   // 19
        "EXX",  // 20
};

const Regs::RegList *RegsZ280::listRegisters(uint_fast8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS8, 10, 1, UINT8_MAX},
            {REGS16, 8, 11, UINT16_MAX},
            {EXCHANGE, 2, 19, 1},
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
        _usp = value;
        break;
    case 14:
        _main.setbc(value);
        break;
    case 15:
        _main.setde(value);
        break;
    case 16:
        _main.sethl(value);
        break;
    case 17:
        _ix = value;
        break;
    case 18:
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
    case 19:
        swap8(_main.a, _alt.a);
        swap8(_main.f, _alt.f);
        break;
    case 20:
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
