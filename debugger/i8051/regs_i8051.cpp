#include "regs_i8051.h"
#include "char_buffer.h"
#include "debugger.h"
#include "inst_i8051.h"
#include "pins_i8051.h"

namespace debugger {
namespace i8051 {

const char *RegsI8051::cpuName() const {
    return _pins->isCmos() ? "P80C51" : "P8051";
}

void RegsI8051::print() const {
    static constexpr char line1[] =
            "PC=xxxx SP=xx PSW=CA111V1P DPTR=xxxx B=xx A=xx RS=x";
    //       012345678901234567890123456789012345678901234567890
    static constexpr char line2[] =
            "R0=xx R1=xx R2=xx R3=xx R4=xx R5=xx R6=xx R7=xx";
    //       0123456789012345678901234567890123456789012345678901
    static auto &buffer = *new CharBuffer(line1);
    buffer.hex16(3, _pc);
    buffer.hex8(11, _sp);
    buffer.bits(18, _psw, 0x80, line1 + 18);
    buffer.hex16(32, _dptr);
    buffer.hex8(39, _b);
    buffer.hex8(44, _a);
    buffer.hex4(50, rs());
    cli.println(buffer);
    static auto &regs = *new CharBuffer(line2);
    for (auto i = 0; i < 8; ++i)
        regs.hex8(3 + i * 6, _r[i]);
    cli.println(regs);
    _pins->idle();
}

void RegsI8051::save() {
    _pins->captureWrites(SAVE_A, sizeof(SAVE_A), &_pc, &_a, sizeof(_a));
    _b = raw_read_internal(B);
    _psw = raw_read_internal(PSW);
    _sp = raw_read_internal(SP);
    _dptr = uint16(raw_read_internal(DPH), raw_read_internal(DPL));
    save_r();
}

void RegsI8051::save_r() {
    for (auto num = 0; num < 8; ++num)
        _r[num] = raw_read_internal(r_base() + num);
    write_internal(ACC, _a);  // restore A
}

void RegsI8051::set_psw(uint8_t val) {
    write_internal(PSW, _psw = val);
    save_r();
}

void RegsI8051::set_rs(uint8_t val) {
    set_psw((_psw & ~0x18) | ((val & 3) << 3));
}

void RegsI8051::set_r(uint8_t num, uint8_t val) {
    write_internal(r_base() + num, _r[num] = val);
}

void RegsI8051::restore() {
    write_internal(DPH, hi(_dptr));
    write_internal(DPL, lo(_dptr));
    write_internal(SP, _sp);
    write_internal(PSW, _psw);
    write_internal(B, _b);
    write_internal(ACC, _a);
    const uint8_t LJMP[] = {
            0x02, hi(_pc), lo(_pc), 0,  // LJMP _pc
    };
    _pins->execInst(LJMP, sizeof(LJMP));
}

uint8_t RegsI8051::raw_read_internal(uint8_t addr) const {
    uint8_t data;
    if (addr == ACC) {
        _pins->captureWrites(
                SAVE_A, sizeof(SAVE_A), nullptr, &data, sizeof(data));
    } else {
        const uint8_t READ[] = {
                0xE5, addr,  // MOV A, addr
                0xF2,        // MOVX @R0, A
        };
        _pins->captureWrites(READ, sizeof(READ), nullptr, &data, sizeof(data));
    }
    return data;
}

uint8_t RegsI8051::read_internal(uint8_t addr) const {
    const auto data = raw_read_internal(addr);
    write_internal(ACC, _a);  // restore A
    return data;
}

void RegsI8051::write_internal(uint8_t addr, uint8_t data) const {
    if (addr == ACC) {
        const uint8_t WRITE_A[] = {
                0x74, data,  // MOV A, #data
        };
        _pins->execInst(WRITE_A, sizeof(WRITE_A));
    } else {
        const uint8_t WRITE[] = {
                0x75, addr, data, data,  // MOV addr, #data
        };
        _pins->execInst(WRITE, sizeof(WRITE));
    }
}

void RegsI8051::helpRegisters() const {
    cli.println("?Reg: PC DPTR A B SP R0~R7 PSW F0~1 RS");
}

constexpr const char *REGS8[] = {
        "PSW",  // 1
        "A",    // 2
        "B",    // 3
        "R0",   // 4
        "R1",   // 5
        "R2",   // 6
        "R3",   // 7
        "R4",   // 8
        "R5",   // 9
        "R6",   // 10
        "R7",   // 11
        "SP",   // 12
};

constexpr const char *REGS16[] = {
        "PC",    // 13
        "DPTR",  // 14
};

constexpr const char *REGS2[] = {
        "RS",  // 15
};

constexpr const char *REGS1[] = {
        "F0",  // 16
        "F1",  // 17
};

const Regs::RegList *RegsI8051::listRegisters(uint8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS8, 12, 1, UINT8_MAX},
            {REGS16, 2, 13, UINT16_MAX},
            {REGS2, 1, 15, 3},
            {REGS1, 2, 16, 1},
    };
    return n < 4 ? &REG_LIST[n] : nullptr;
}

void RegsI8051::setRegister(uint8_t reg, uint32_t value) {
    switch (reg) {
    case 1:
        set_psw(value);
        break;
    case 2:
        write_internal(ACC, _a = value);
        break;
    case 3:
        write_internal(B, _b = value);
        break;
    case 12:
        write_internal(SP, _sp = value);
        break;
    case 13:
        _pc = value;
        break;
    case 14:
        _dptr = value;
        write_internal(DPL, lo(_dptr));
        write_internal(DPH, hi(_dptr));
        break;
    case 15:
        set_rs(value);
        break;
    case 16:
        _psw &= ~0x20;
        _psw |= (value << 5);
        break;
    case 17:
        _psw &= ~0x02;
        _psw |= (value << 1);
        break;
    default:
        _r[reg - 4] = value;
        break;
    }
}

}  // namespace i8051
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
