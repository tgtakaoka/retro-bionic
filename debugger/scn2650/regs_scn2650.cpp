#include "regs_scn2650.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "mems_scn2650.h"
#include "pins_scn2650.h"

namespace debugger {
namespace scn2650 {

struct RegsScn2650 Regs;

const char *RegsScn2650::cpu() const {
    return "2650";
}

const char *RegsScn2650::cpuName() const {
    return "SCN2650";
}

void RegsScn2650::print() const {
    static constexpr char line[] =
            "PC=xxxx PSU=SFI11111 PSL=11D1WVLC SP=x CC=x RS=x";
    //       012345678901234567890123456789012345678901234567
    static auto &buffer = *new CharBuffer(line);
    buffer.hex16(3, _pc);
    buffer.bits(12, _psu, 0x80, line + 12);
    buffer.bits(25, _psl, 0x80, line + 25);
    buffer[31] = (_psl & 2) ? 'L' : 'A';  // Logic ot Arithmetic
    buffer.hex4(37, _psu & 7);            // Stack pointer
    static constexpr char cc[] = {'Z', 'P', 'N', '3'};
    buffer[42] = cc[_psl >> 6];  // Condition code
    buffer.hex1(47, rs());       // Register select
    cli.println(buffer);
    static constexpr char regs[] =
            "R0=xx R1=xx R2=xx R3=xx (R1=xx R2=xx R3=xx)";
    //       0123456789012345678901234567890123456789012
    static auto &regbuf = *new CharBuffer(regs);
    regbuf.hex8(3, _r0);
    const auto bank = rs();
    for (auto n = 0; n < 3; ++n) {
        regbuf.hex8(n * 6 + 9, _r[bank][n]);
        regbuf.hex8(n * 6 + 28, _r[1 - bank][n]);
    }
    cli.println(regbuf);
    Pins.idle();
}

void RegsScn2650::save() {
    static constexpr uint8_t SAVE_R0PS[] = {
            0xC8, 0x00,        // STRR,R0 $
            0x13, 0xC8, 0x00,  // SPSL, STRR,R0 $
            0x12, 0xC8, 0x00,  // SPSU; STRR,R0 $
    };
    uint8_t buffer[3];
    Pins.captureWrites(
            SAVE_R0PS, sizeof(SAVE_R0PS), &_pc, buffer, sizeof(buffer));
    _r0 = buffer[0];
    _psl = buffer[1];
    _psu = buffer[2];
    static constexpr uint8_t SAVE_REGS[] = {
            0xC9, 0x00,  // STRR,R1 $
            0xCA, 0x00,  // STRR,R2,$
            0xCB, 0x00,  // STRR,R3 $
    };
    Pins.captureWrites(SAVE_REGS, sizeof(SAVE_REGS), nullptr, &_r[rs()][0], 3);
    constexpr uint8_t PPSL = 0x77;
    constexpr uint8_t CPSL = 0x75;
    static uint8_t SET_RS[] = {
            0, 0x10,  // PPSL|CPSL 0x10
    };
    SET_RS[0] = rs() ? CPSL : PPSL;
    Pins.execInst(SET_RS, sizeof(SET_RS));
    Pins.captureWrites(
            SAVE_REGS, sizeof(SAVE_REGS), nullptr, &_r[1 - rs()][0], 3);
    SET_RS[0] = rs() ? PPSL : CPSL;
    Pins.execInst(SET_RS, sizeof(SET_RS));
}

void RegsScn2650::restore() {
    constexpr uint8_t PPSL = 0x77;
    constexpr uint8_t CPSL = 0x75;
    static uint8_t RESTORE[] = {
            0, 0x10,        // PPSL|CPSL 0x10; switch to !rs bank
            0x05, 0,        // LODI,R1 _r1
            0x06, 0,        // LODI,R2 _r2
            0x07, 0,        // LODI,R3 _r3
            0, 0x10,        // PPSL|CPSL 0x10; switch to rs bank
            0x05, 0,        // LODI,R1 _r1
            0x06, 0,        // LODI,R2 _r2
            0x07, 0,        // LODI,R3 _r3
            0x04, 0, 0x92,  // LODI,R0 _psu; LPSU
            0x04, 0,        // LODI,R0 _r0
            PPSL, 0,        // PPSL _psl; restore PSL one bits
            CPSL, 0,        // CPSL ~_psl; restore PSL zero bits
            0x1F, 0, 0,     // BCTA _pc
    };
    RESTORE[0] = rs() ? CPSL : PPSL;
    RESTORE[8] = rs() ? PPSL : CPSL;
    for (auto n = 0; n < 3; ++n) {
        RESTORE[n * 2 + 3] = _r[1 - rs()][n];
        RESTORE[n * 2 + 11] = _r[rs()][n];
    }
    RESTORE[17] = _psu;
    RESTORE[20] = _r0;
    RESTORE[22] = _psl;
    RESTORE[24] = ~_psl;
    RESTORE[26] = hi(_pc);
    RESTORE[27] = lo(_pc);
    Pins.execInst(RESTORE, sizeof(RESTORE));
}

uint8_t RegsScn2650::read_io(uint8_t addr) const {
    static uint8_t REDE[] = {
            0x54, 0,     // REDE,R0 addr
            0xC8, 0x00,  // STRR,R0 $
    };
    REDE[1] = addr;
    uint8_t data;
    Pins.captureWrites(REDE, sizeof(REDE), nullptr, &data, sizeof(data));
    return data;
}

void RegsScn2650::write_io(uint8_t addr, uint8_t data) const {
    static uint8_t WRTE[] = {
            0x04, 0,  // LODI,R0 data
            0xD4, 0,  // WRTE,R0 addr
    };
    WRTE[1] = data;
    WRTE[3] = addr;
    Pins.execInst(WRTE, sizeof(WRTE));
}

void RegsScn2650::helpRegisters() const {
    cli.println(F("?Reg: PC PSU/PSL RS CC R0~R3"));
}

constexpr const char *REGS1[] = {
        "RS",  // 1
};
constexpr const char *REGS2[] = {
        "CC",  // 2
};
constexpr const char *REGS8[] = {
        "R0",   // 3
        "R1",   // 4
        "R2",   // 5
        "R3",   // 6
        "PSU",  // 7
        "PSL",  // 8
};
constexpr const char *REGS15[] = {
        "PC",  // 9
};

const Regs::RegList *RegsScn2650::listRegisters(uint8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS1, 1, 1, 1},
            {REGS2, 1, 2, 0x3},
            {REGS8, 6, 3, UINT8_MAX},
            {REGS15, 1, 9, 0x7FFF},
    };
    return n < 4 ? &REG_LIST[n] : nullptr;
}

void RegsScn2650::setRegister(uint8_t reg, uint32_t value) {
    switch (reg) {
    case 9:
        _pc = value;
        break;
    case 7:
        _psu = value;
        break;
    case 2:
        value <<= 6;
        value |= _psl & 0x3F;
        // Fall-through
    case 8:
        _psl = value;
        break;
    case 1:
        if (value)
            _psl |= 0x10;
        else
            _psl &= ~0x10;
        break;
    case 3:
        _r0 = value;
        break;
    default:
        _r[rs()][reg - 4] = value;
        break;
    }
}

}  // namespace scn2650
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
