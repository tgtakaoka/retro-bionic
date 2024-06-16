#include "regs_z80.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "inst_z80.h"
#include "mems_z80.h"
#include "pins_z80.h"

namespace debugger {
namespace z80 {

struct RegsZ80 Regs;

const char *RegsZ80::cpu() const {
    return "Z80";
}

const char *RegsZ80::cpuName() const {
    return cpu();
}

void RegsZ80::print() const {
    // clang-format off
    //                              01234567890123456789012345678901234567890123456789012345678901
    static constexpr char main[] = "PC=xxxx SP=xxxx  BC=xxxx DE=xxxx HL=xxxx A=xx F=SZ1H1VNC  I=xx";
    static constexpr char alt[]  = "IX=xxxx IY=xxxx (BC=xxxx DE=xxxx HL=xxxx A=xx F=SZ1H1VNC) R=xx";
    // clang-format on
    static auto &bufmain = *new CharBuffer(main);
    static auto &bufalt = *new CharBuffer(alt);
    bufmain.hex16(3, _pc);
    bufmain.hex16(11, _sp);
    bufmain.hex16(20, _main.bc());
    bufmain.hex16(28, _main.de());
    bufmain.hex16(36, _main.hl());
    bufmain.hex8(43, _main.a);
    bufmain.bits(48, _main.f, 0x80, main + 48);
    bufmain.hex8(60, _i);
    cli.println(bufmain);
    Pins.idle();
    bufalt.hex16(3, _ix);
    bufalt.hex16(11, _iy);
    bufalt.hex16(20, _alt.bc());
    bufalt.hex16(28, _alt.de());
    bufalt.hex16(36, _alt.hl());
    bufalt.hex8(43, _alt.a);
    bufalt.bits(48, _alt.f, 0x80, alt + 48);
    bufalt.hex8(60, _r);
    cli.println(bufalt);
    Pins.idle();
}

void RegsZ80::exchangeRegs() {
    static constexpr uint8_t EXCHANGE[] = {
            0x08,  // EX AF, AF'
            0xD9,  // EXX
    };
    Pins.execInst(EXCHANGE, sizeof(EXCHANGE));
}

void RegsZ80::saveRegs(reg &regs) {
    static constexpr uint8_t PUSH_ALL[] = {
            0xF5,  // PUSH AF
            0xC5,  // PUSH BC
            0xD5,  // PUSH DE
            0xE5,  // PUSH HL
    };
    Pins.captureWrites(PUSH_ALL, sizeof(PUSH_ALL), nullptr, (uint8_t *)&regs,
            sizeof(regs));
}

void RegsZ80::restoreRegs(const reg &regs) {
    uint8_t POP_ALL[] = {
            0xF1, regs.f, regs.a,  // POP AF
            0xC1, regs.c, regs.b,  // POP BC
            0xD1, regs.e, regs.d,  // POP DE
            0xE1, regs.l, regs.h,  // POP HL
    };
    Pins.execInst(POP_ALL, sizeof(POP_ALL));
}

void RegsZ80::save() {
    static constexpr uint8_t PUSH_PC[] = {
            0xC7,  // RST 0
    };
    uint8_t buffer[6];
    Pins.captureWrites(PUSH_PC, sizeof(PUSH_PC), &_sp, buffer, sizeof(_pc));
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
            0xED, 0x5F,  // LD A, R
            0x77,        // LD (HL), A
    };
    Pins.captureWrites(
            SAVE_OTHERS, sizeof(SAVE_OTHERS), nullptr, buffer, sizeof(buffer));
    _ix = be16(buffer + 0);
    _iy = be16(buffer + 2);
    _i = buffer[4];
    _r = buffer[5];
}

void RegsZ80::restore() {
    uint8_t LD_OTHERS[] = {
            0x3E, _r,                // LD A, _r
            0xED, 0x4F,              // LD R, A
            0x3E, _i,                // LD A, _i
            0xED, 0x47,              // LD A, I
            0xFD, lo(_iy), hi(_iy),  // POP IY, _iy
            0xDD, lo(_ix), hi(_ix),  // POP IX, _ix
    };
    Pins.execInst(LD_OTHERS, sizeof(LD_OTHERS));
    restoreRegs(_main);
    exchangeRegs();
    restoreRegs(_alt);
    exchangeRegs();
    uint8_t LD_ALL[] = {
            0x31, lo(_sp), hi(_sp),  // LD SP, _sp
            0xC3, lo(_pc), hi(_pc),  // JP _pc
    };
    Pins.execInst(LD_ALL, sizeof(LD_ALL));
}

uint8_t RegsZ80::read_io(uint8_t addr) const {
    uint8_t IN[] = {
            0xDB, addr,  // IN (addr)
            0x77,        // LD (HL), A
    };
    uint8_t data;
    Pins.captureWrites(IN, sizeof(IN), nullptr, &data, sizeof(data));
    return data;
}

void RegsZ80::write_io(uint8_t addr, uint8_t data) const {
    uint8_t OUT[] = {
            0x3E, data,  // LD A, data
            0xD3, addr,  // OUT (addr)
            0x77,        // LD (HL), A
    };
    uint8_t tmp;
    // LD (HL), A ensures I/O write cycle.
    Pins.captureWrites(OUT, sizeof(OUT), nullptr, &tmp, sizeof(tmp));
}

void RegsZ80::helpRegisters() const {
    cli.println(F("?Reg: PC SP IX IY BC DE HL A F B C D E H L I R EX EXX"));
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

const Regs::RegList *RegsZ80::listRegisters(uint8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS8, 10, 1, UINT8_MAX},
            {REGS16, 7, 11, UINT16_MAX},
            {EXCHANGE, 2, 18, 1},
    };
    return n < 3 ? &REG_LIST[n] : nullptr;
}

void RegsZ80::setRegister(uint8_t reg, uint32_t value) {
    switch (reg) {
    case 11:
        _pc = value;
        break;
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
}

}  // namespace z80
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
