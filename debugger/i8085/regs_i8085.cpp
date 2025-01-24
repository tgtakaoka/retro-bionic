#include "regs_i8085.h"
#include "debugger.h"
#include "inst_i8085.h"
#include "pins_i8085.h"

namespace debugger {
namespace i8085 {

namespace {
const char line[] =
        "PC=xxxx SP=xxxx BC=xxxx DE=xxxx HL=xxxx A=xx PSW=SZ#H#P#C IE";
//       012345678901234567890123456789012345678901234567890123456789
}  // namespace

RegsI8085::RegsI8085(PinsI8085 *pins) : _pins(pins), _buffer(line) {}

const char *RegsI8085::cpu() const {
    return "i8085";
}

void RegsI8085::print() const {
    _buffer.hex16(3, _pc);
    _buffer.hex16(11, _sp);
    _buffer.hex8(19, _b);
    _buffer.hex8(21, _c);
    _buffer.hex8(27, _d);
    _buffer.hex8(29, _e);
    _buffer.hex8(35, _h);
    _buffer.hex8(37, _l);
    _buffer.hex8(42, _a);
    _buffer.bits(49, _psw, 0x80, line + 49);
    _buffer[57] = _ie ? ' ' : 0;
    cli.println(_buffer);
    _pins->idle();
}

void RegsI8085::save() {
    static const uint8_t PUSH_ALL[] = {
            0xC7,  // RST 0
            0xF5,  // PUSH PSW
            0x20,  // RIM
            0xF3,  // DI
            0xC5,  // PUSH B
            0xD5,  // PUSH D
            0xE5,  // PUSH H
            0x77,  // MOV M, A
    };
    uint8_t buffer[11];
    _pins->captureWrites(
            PUSH_ALL, sizeof(PUSH_ALL), &_sp, buffer, sizeof(buffer));
    _sp++;
    _pc = be16(buffer) - 1;  // offser RST instruction
    _a = buffer[2];
    _psw = buffer[3];
    _b = buffer[4];
    _c = buffer[5];
    _d = buffer[6];
    _e = buffer[7];
    _h = buffer[8];
    _l = buffer[9];
    _ie = (buffer[10] & 0x08) != 0;
}

void RegsI8085::restore() {
    const auto ie = _ie ? InstI8085::EI : InstI8085::DI;
    const uint8_t POP_ALL[] = {
            0x01, _c, _b,            // LXI B, _bc
            0x11, _e, _d,            // LXI D, _de
            0x21, _l, _h,            // LXI H, _hl
            0xF1, _psw, _a,          // POP PSW
            0x31, lo(_sp), hi(_sp),  // LXI SP, _sp
            ie,                      // EI/DI
            0xC3, lo(_pc), hi(_pc),  // JMP _pc
    };
    _pins->execInst(POP_ALL, sizeof(POP_ALL));
}

uint8_t RegsI8085::read_io(uint8_t addr) const {
    const uint8_t IN[] = {
            0xDB, addr,  // IN addr
            0x77,        // MOV M, A
    };
    uint8_t data;
    _pins->captureWrites(IN, sizeof(IN), nullptr, &data, sizeof(data));
    return data;
}

void RegsI8085::write_io(uint8_t addr, uint8_t data) const {
    const uint8_t OUT[] = {
            0x3E, data,  // MVI data
            0xD3, addr,  // OUT addr
            0x77,        // MOV M, A
    };
    uint8_t tmp;
    // MOV M, A ensures I/O write cycle.
    _pins->captureWrites(OUT, sizeof(OUT), nullptr, &tmp, sizeof(tmp));
}

void RegsI8085::helpRegisters() const {
    cli.println("?Reg: PC SP BC DE HL A B C D E H L PSW");
}

constexpr const char *REGS8[] = {
        "PSW",  // 1
        "A",    // 2
        "B",    // 3
        "C",    // 4
        "D",    // 5
        "E",    // 6
        "H",    // 7
        "L",    // 8
};
constexpr const char *REGS16[] = {
        "PC",  // 9
        "SP",  // 10
        "BC",  // 11
        "DE",  // 12
        "HL",  // 13
};

const Regs::RegList *RegsI8085::listRegisters(uint_fast8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS8, 8, 1, UINT8_MAX},
            {REGS16, 5, 9, UINT16_MAX},
    };
    return n < 2 ? &REG_LIST[n] : nullptr;
}

bool RegsI8085::setRegister(uint_fast8_t reg, uint32_t value) {
    switch (reg) {
    case 9:
        _pc = value;
        return true;
    case 10:
        _sp = value;
        break;
    case 11:
        _bc(value);
        break;
    case 12:
        _de(value);
        break;
    case 13:
        _hl(value);
        break;
    case 2:
        _a = value;
        break;
    case 3:
        _b = value;
        break;
    case 4:
        _c = value;
        break;
    case 5:
        _d = value;
        break;
    case 6:
        _e = value;
        break;
    case 7:
        _h = value;
        break;
    case 8:
        _l = value;
        break;
    case 1:
        _psw = value;
        break;
    }
    return false;
}

}  // namespace i8085
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
