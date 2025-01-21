#include "regs_i8048.h"
#include "debugger.h"
#include "inst_i8048.h"
#include "pins_i8048.h"

namespace debugger {
namespace i8048 {

namespace {
const char I8039[] = "i8039";
const char MSM80C39[] = "MSM80C39";
//                              1         2         3         4
//                    0123456789012345678901234567890123456789012345678
const char line1[] = "PC=xxx MB=x PSW=CAFB#### F0=x F1=x SP=x BS=x A=xx";
const char line2[] = "R0=xx R1=xx R2=xx R3=xx R4=xx R5=xx R6=xx R7=xx";
}  // namespace

RegsI8048::RegsI8048(PinsI8048 *pins)
    : _pins(pins), _buffer1(line1), _buffer2(line2) {}

const char *RegsI8048::cpu() const {
    return _pins->softwareType() == SW_I8048 ? I8039 : MSM80C39;
}

const char *RegsI8048::cpuName() const {
    return _pins->softwareType() == SW_I8048 ? P8039 : MSM80C39;
}

void RegsI8048::print() const {
    _buffer1.hex12(3, _pc);
    _buffer1.hex1(10, _mb);
    _buffer1.bits(16, _psw, 0x80, line1 + 16);
    _buffer1.hex1(28, _psw & f0);
    _buffer1.hex1(33, _f1);
    _buffer1.hex4(38, _psw & sp);
    _buffer1.hex1(43, _psw & bs);
    _buffer1.hex8(47, _a);
    cli.println(_buffer1);
    for (auto i = 0; i < 8; ++i)
        _buffer2.hex8(3 + i * 6, _r[i]);
    cli.println(_buffer2);
    _pins->idle();
}

void RegsI8048::save() {
    static const uint8_t SAVE[] = {
            0x90,  // MOVX @R0, A
            0xC7,  // MOV A, PSW
            0x90,  // MOVX @R0, A
    };
    uint8_t buffer[2];
    _pins->captureWrites(SAVE, sizeof(SAVE), &_pc, buffer, sizeof(buffer));
    _a = buffer[0];
    _psw = buffer[1];
    _pins->cycle(InstI8048::JMP(_pc));
    _pins->cycle(0);
    const auto mb = _pins->cycle(InstI8048::JF1);
    _mb = (mb->addr & 0x800) != 0;
    _pins->cycle(mb->addr + 10);
    const auto f1 = _pins->cycle(InstI8048::NOP);
    _f1 = (f1->addr == mb->addr + 10);
    save_r();
}

void RegsI8048::restore() {
    restore_r();
    const uint8_t RESTORE[] = {
            0x23, _psw,                // MOV A, #_psw
            0xD7,                      // MOV PSW, A
            0x23, _a,                  // MOV A, #_a
            0xA5,                      // CLR F1
            uint8(_f1 ? 0xB5 : 0x00),  // CPL F1/NOP
    };
    _pins->execInst(RESTORE, sizeof(RESTORE));
    restore_pc(_pc, _mb);
}

void RegsI8048::save_r() {
    for (auto i = 0; i < 8; ++i) {
        const uint8_t SAVE[] = {
                uint8(0xF8 | i),  // MOV A, Ri
                0x90,             // MOVX @R0, A
        };
        _pins->captureWrites(SAVE, sizeof(SAVE), nullptr, &_r[i], 1);
    }
}

void RegsI8048::restore_r() const {
    for (auto i = 0; i < 8; ++i)
        set_r(i, _r[i]);
}

void RegsI8048::restore_pc(uint16_t pc, uint8_t mb) const {
    // Bit 11 is not altered by normal incrementing of the program
    // counter.
    const auto offset = InstI8048::offset(pc - 1);
    const uint8_t JUMP[] = {
            InstI8048::SEL_MB(InstI8048::mb(pc)),
            InstI8048::JMP(offset),
            lo(offset),
            InstI8048::SEL_MB(mb),
    };
    _pins->execInst(JUMP, sizeof(JUMP));
}

void RegsI8048::set_psw(uint8_t val) {
    _psw = val;
    set_rb(val & bs);
}

void RegsI8048::set_r(uint8_t no, uint8_t val) const {
    const uint8_t SET[2] = {
            uint8_t(0xB8 + no),
            val,
    };
    _pins->execInst(SET, sizeof(SET));
}

void RegsI8048::set_rb(uint8_t val) {
    if (val) {
        _psw |= bs;
    } else {
        _psw &= ~bs;
    }
    const uint8_t SEL_RB = val ? 0xD5 : 0xC5;
    _pins->execInst(&SEL_RB, sizeof(SEL_RB));
    save_r();
}

uint8_t RegsI8048::read_internal(uint8_t addr) const {
    if (addr == r_base())
        return _r[0];
    const uint8_t READ[] = {
            0xB8, addr,   // MOV R0, #addr
            0xF0,         // MOV A, @R0
            0x90,         // MOVX @R0, A
            0xB8, _r[0],  // MOV R0, #r0
    };
    uint8_t data;
    _pins->captureWrites(READ, sizeof(READ), nullptr, &data, sizeof(data));
    return data;
}

void RegsI8048::write_internal(uint8_t addr, uint8_t data) const {
    if (addr == r_base()) {
        set_r(0, data);
        return;
    }
    const uint8_t WRITE[] = {
            0xF8,        // MOV A, R0
            0xB8, addr,  // MOV R0, #addr
            0xB0, data,  // MOV @R0, #data
            0xA8,        // MOV R0, A

    };
    _pins->execInst(WRITE, sizeof(WRITE));
}

void RegsI8048::helpRegisters() const {
    cli.println("?Reg: PC MB A R0~R7 PSW F0 F1 BS");
}

constexpr const char *REGS8[] = {
        "PSW",  // 1
        "A",    // 2
        "R0",   // 3
        "R1",   // 4
        "R2",   // 5
        "R3",   // 6
        "R4",   // 7
        "R5",   // 8
        "R6",   // 9
        "R7",   // 10
};
constexpr const char *REGS12[] = {
        "PC",  // 11
};
constexpr const char *REGS1[] = {
        "BS",  // 12
        "MB",  // 13
        "F0",  // 14
        "F1",  // 15
};

const Regs::RegList *RegsI8048::listRegisters(uint8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS8, 10, 1, UINT8_MAX},
            {REGS12, 1, 11, 0xFFF},
            {REGS1, 4, 12, 1},
    };
    return n < 3 ? &REG_LIST[n] : nullptr;
}

void RegsI8048::setRegister(uint8_t reg, uint32_t value) {
    switch (reg) {
    case 1:
        set_psw(value);
        break;
    case 2:
        _a = value;
        break;
    case 11:
        _pc = value;
        break;
    case 12:
        set_rb(value);
        break;
    case 13:
        _mb = value;
        break;
    case 14:
        if (value) {
            _psw |= f0;
        } else {
            _psw &= ~f0;
        }
        break;
    case 15:
        _f1 = value;
        break;
    default:
        _r[reg - 3] = value;
        set_r(reg - 3, value);
        break;
    }
}

}  // namespace i8048
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
