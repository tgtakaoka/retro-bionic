#include "regs_i8080.h"
#include "debugger.h"
#include "inst_i8080.h"
#include "pins_i8080.h"

namespace debugger {
namespace i8080 {

namespace {
const char line[] =
        "PC=xxxx SP=xxxx BC=xxxx DE=xxxx HL=xxxx A=xx PSW=SZ#H#P#C IE=x";
//       01234567890123456789012345678901234567890123456789012345678901
}  // namespace

RegsI8080::RegsI8080(PinsI8080Base *pins) : _pins(pins), _buffer(line) {}

const char *RegsI8080::cpu() const {
    return "i8080";
}

void RegsI8080::print() const {
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
    _buffer.hex1(61, _ie);
    cli.println(_buffer);
    _pins->idle();
}

bool RegsI8080::ie() const {
    return digitalReadFast(PIN_INTE) != LOW;
}

void RegsI8080::save() {
    static const uint8_t PUSH_ALL[] = {
            0xC7,  // RST 0
            0xF5,  // PUSH PSW
            0xC5,  // PUSH B
            0xD5,  // PUSH D
            0xE5,  // PUSH H
    };
    uint8_t buffer[10];
    _sp = _pins->captureWrites(
            PUSH_ALL, sizeof(PUSH_ALL), buffer, sizeof(buffer));
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
    _ie = ie();
}

void RegsI8080::saveContext(uint16_t pc, uint8_t inte) {
    save();
    _pc = pc;
    _ie = (inte != 0);
}

void RegsI8080::restore() {
    const auto ie = _ie ? InstI8080::EI : InstI8080::DI;
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

uint8_t RegsI8080::read_io(uint8_t addr) const {
    const uint8_t IN[] = {
            0xDB, addr,  // IN addr
            0x77,        // MOV M, A
    };
    uint8_t data;
    _pins->captureWrites(IN, sizeof(IN), &data, sizeof(data));
    return data;
}

void RegsI8080::write_io(uint8_t addr, uint8_t data) const {
    const uint8_t OUT[] = {
            0x3E, data,  // MVI data
            0xD3, addr,  // OUT addr
            0x77,        // MOV M, A
    };
    uint8_t tmp;
    // MOV M, A ensures I/O write cycle.
    _pins->captureWrites(OUT, sizeof(OUT), &tmp, sizeof(tmp));
}

void RegsI8080::helpRegisters() const {
    cli.println("?Reg: PC SP BC DE HL A B C D E H L PSW IE");
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
constexpr const char *REGS1[] = {
        "IE",  // 14
};

const Regs::RegList *RegsI8080::listRegisters(uint_fast8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS8, 8, 1, UINT8_MAX},
            {REGS16, 5, 9, UINT16_MAX},
            {REGS1, 1, 14, 1},
    };
    return n < 3 ? &REG_LIST[n] : nullptr;
}

bool RegsI8080::setRegister(uint_fast8_t reg, uint32_t value) {
    switch (reg) {
    case 1:
        _psw = value;
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
    case 14:
        _ie = value;
        break;
    }
    return false;
}

}  // namespace i8080
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
