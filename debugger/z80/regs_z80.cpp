#include "regs_z80.h"
#include "debugger.h"
#include "digital_bus.h"
#include "inst_z80.h"
#include "mems_z80.h"

namespace debugger {
namespace z80 {
namespace {
// clang-format off
//                              1         2         3         4         5         6
//                    0123456789012345678901234567890123456789012345678901234567890
const char line1[] = "PC=xxxx SP=xxxx  BC=xxxx DE=xxxx HL=xxxx A=xx F=SZ1H1VNC I=xx";
const char line2[] = "IX=xxxx IY=xxxx (BC=xxxx DE=xxxx HL=xxxx A=xx F=SZ1H1VNC)";
// clang-format on
}  // namespace

RegsZ80::RegsZ80(PinsZ80Base *pins)
    : _pins(pins), _buffer1(line1), _buffer2(line2) {}

const char *RegsZ80::cpu() const {
    return "Z80";
}

void RegsZ80::print() const {
    auto main = _buffer1;
    main.hex16(3, _pc);
    main.hex16(11, _sp);
    main.hex16(20, _main.bc());
    main.hex16(28, _main.de());
    main.hex16(36, _main.hl());
    main.hex8(43, _main.a);
    main.bits(48, _main.f, 0x80, main + 48);
    main.hex8(59, _i);
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
    cli.println(alt);
    _pins->idle();
}

void RegsZ80::exchangeRegs() const {
    static constexpr uint8_t EXCHANGE[] = {
            0x08,  // EX AF, AF'
            0xD9,  // EXX
    };
    _pins->execInst(EXCHANGE, sizeof(EXCHANGE));
}

void RegsZ80::saveRegs(reg &regs) const {
    static constexpr uint8_t PUSH_ALL[] = {
            0xF5,  // PUSH AF
            0xC5,  // PUSH BC
            0xD5,  // PUSH DE
            0xE5,  // PUSH HL
    };
    _pins->captureWrites(
            PUSH_ALL, sizeof(PUSH_ALL), (uint8_t *)&regs, sizeof(regs));
}

void RegsZ80::restoreRegs(const reg &regs) const {
    const uint8_t POP_ALL[] = {
            0xF1, regs.f, regs.a,  // POP AF
            0xC1, regs.c, regs.b,  // POP BC
            0xD1, regs.e, regs.d,  // POP DE
            0xE1, regs.l, regs.h,  // POP HL
    };
    _pins->execInst(POP_ALL, sizeof(POP_ALL));
}

void RegsZ80::save() {
    static constexpr uint8_t PUSH_PC[] = {
            0xC7,  // RST 0
    };
    uint8_t buffer[5];
    _sp = _pins->captureWrites(PUSH_PC, sizeof(PUSH_PC), buffer, sizeof(_pc));
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
    _pins->captureWrites(
            SAVE_OTHERS, sizeof(SAVE_OTHERS), buffer, sizeof(buffer));
    _ix = be16(buffer + 0);
    _iy = be16(buffer + 2);
    _i = buffer[4];
}

void RegsZ80::restore() {
    const uint8_t LD_OTHERS[] = {
            0x3E, _i,                      // LD A, _i
            0xED, 0x47,                    // LD A, I
            0xFD, 0xE1, lo(_iy), hi(_iy),  // POP IY, _iy
            0xDD, 0xE1, lo(_ix), hi(_ix),  // POP IX, _ix
    };
    _pins->execInst(LD_OTHERS, sizeof(LD_OTHERS));
    restoreRegs(_main);
    exchangeRegs();
    restoreRegs(_alt);
    exchangeRegs();
    const uint8_t LD_ALL[] = {
            0x31, lo(_sp), hi(_sp),  // LD SP, _sp
            0xC3, lo(_pc), hi(_pc),  // JP _pc
    };
    _pins->execInst(LD_ALL, sizeof(LD_ALL));
}

void RegsZ80::helpRegisters() const {
    cli.println("?Reg: PC SP IX IY BC DE HL A F B C D E H L I EX EXX");
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
};
constexpr const char *REGS16[] = {
        "PC",  // 10
        "SP",  // 11
        "BC",  // 12
        "DE",  // 13
        "HL",  // 14
        "IX",  // 15
        "IY",  // 16
};
constexpr const char *EXCHANGE[] = {
        "EX",   // 17
        "EXX",  // 18
};

const Regs::RegList *RegsZ80::listRegisters(uint_fast8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS8, 9, 1, UINT8_MAX},
            {REGS16, 7, 10, UINT16_MAX},
            {EXCHANGE, 2, 17, 1},
    };
    return n < 3 ? &REG_LIST[n] : nullptr;
}

bool RegsZ80::setRegister(uint_fast8_t reg, uint32_t value) {
    switch (reg) {
    case 10:
        _pc = value;
        return true;
    case 11:
        _sp = value;
        break;
    case 12:
        _main.setbc(value);
        break;
    case 13:
        _main.setde(value);
        break;
    case 14:
        _main.sethl(value);
        break;
    case 15:
        _ix = value;
        break;
    case 16:
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
    case 17:
        swap8(_main.a, _alt.a);
        swap8(_main.f, _alt.f);
        break;
    case 18:
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

}  // namespace z80
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
