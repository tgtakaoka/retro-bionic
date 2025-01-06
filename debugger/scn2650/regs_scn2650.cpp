#include "regs_scn2650.h"
#include "debugger.h"
#include "digital_bus.h"
#include "mems_scn2650.h"
#include "pins_scn2650.h"

namespace debugger {
namespace scn2650 {
namespace {
//                              1         2         3         4
//                    012345678901234567890123456789012345678901234567
const char line1[] = "PC=xxxx PSU=SFI11111 PSL=11D1WVLC SP=x CC=x RS=x";
const char line2[] = "R0=xx R1=xx R2=xx R3=xx (R1=xx R2=xx R3=xx)";
}  // namespace

RegsScn2650::RegsScn2650(PinsScn2650 *pins)
    : _pins(pins), _buffer1(line1), _buffer2(line2) {}

const char *RegsScn2650::cpu() const {
    return "2650";
}

const char *RegsScn2650::cpuName() const {
    return "SCN2650";
}

void RegsScn2650::print() const {
    _buffer1.hex16(3, _pc);
    _buffer1.bits(12, _psu, 0x80, line1 + 12);
    _buffer1.bits(25, _psl, 0x80, line1 + 25);
    _buffer1[31] = (_psl & 2) ? 'L' : 'A';  // Logic ot Arithmetic
    _buffer1.hex4(37, _psu & 7);            // Stack pointer
    static constexpr char cc[] = {'Z', 'P', 'N', '3'};
    _buffer1[42] = cc[_psl >> 6];  // Condition code
    _buffer1.hex1(47, rs());       // Register select
    cli.println(_buffer1);
    _buffer2.hex8(3, _r0);
    const auto bank = rs();
    for (auto n = 0; n < 3; ++n) {
        _buffer2.hex8(n * 6 + 9, _r[bank][n]);
        _buffer2.hex8(n * 6 + 28, _r[1 - bank][n]);
    }
    cli.println(_buffer2);
    _pins->idle();
}

void RegsScn2650::setRs(uint8_t rs) const {
    const uint8_t SET_RS[] = {
            rs, 0x10,  // PPSL|CPSL 0x10
    };
    _pins->execInst(SET_RS, sizeof(SET_RS));
}

void RegsScn2650::saveRegs(uint8_t *regs) const {
    static constexpr uint8_t SAVE[] = {
            0xC9, 0x00,  // STRR,R1 $
            0xCA, 0x00,  // STRR,R2,$
            0xCB, 0x00,  // STRR,R3 $
    };
    _pins->captureWrites(SAVE, sizeof(SAVE), nullptr, regs, 3);
}

void RegsScn2650::restoreRegs(const uint8_t *regs) const {
    const uint8_t RESTORE[] = {
            0x05, regs[0],  // LODI,R1 _r1
            0x06, regs[1],  // LODI,R2 _r2
            0x07, regs[2],  // LODI,R3 _r3
    };
    _pins->execInst(RESTORE, sizeof(RESTORE));
}

void RegsScn2650::save() {
    static constexpr uint8_t SAVE_R0PS[] = {
            0xC8, 0x00,        // STRR,R0 $
            0x13, 0xC8, 0x00,  // SPSL, STRR,R0 $
            0x12, 0xC8, 0x00,  // SPSU; STRR,R0 $
    };
    uint8_t buffer[3];
    _pins->captureWrites(
            SAVE_R0PS, sizeof(SAVE_R0PS), &_pc, buffer, sizeof(buffer));
    _r0 = buffer[0];
    _psl = buffer[1];
    _psu = buffer[2];
    saveRegs(_r[rs()]);
    setRs(rs() ? CPSL : PPSL);
    saveRegs(_r[1 - rs()]);
    setRs(rs() ? PPSL : CPSL);
}

void RegsScn2650::restore() {
    restoreRegs(_r[rs()]);
    setRs(rs() ? CPSL : PPSL);
    restoreRegs(_r[1 - rs()]);
    setRs(rs() ? PPSL : CPSL);
    const uint8_t RESTORE[] = {
            0x04, _psu, 0x92,        // LODI,R0 _psu; LPSU
            0x04, _r0,               // LODI,R0 _r0
            PPSL, _psl,              // PPSL _psl; restore PSL one bits
            CPSL, uint8(~_psl),      // CPSL ~_psl; restore PSL zero bits
            0x1F, hi(_pc), lo(_pc),  // BCTA _pc
    };
    _pins->execInst(RESTORE, sizeof(RESTORE));
}

uint8_t RegsScn2650::read_io(uint8_t addr) const {
    const uint8_t REDE[] = {
            0x54, addr,  // REDE,R0 addr
            0xC8, 0x00,  // STRR,R0 $
    };
    uint8_t data;
    _pins->captureWrites(REDE, sizeof(REDE), nullptr, &data, sizeof(data));
    return data;
}

void RegsScn2650::write_io(uint8_t addr, uint8_t data) const {
    const uint8_t WRTE[] = {
            0x04, data,  // LODI,R0 data
            0xD4, addr,  // WRTE,R0 addr
    };
    _pins->execInst(WRTE, sizeof(WRTE));
}

void RegsScn2650::helpRegisters() const {
    cli.println("?Reg: PC PSU/PSL RS CC R0~R3");
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
