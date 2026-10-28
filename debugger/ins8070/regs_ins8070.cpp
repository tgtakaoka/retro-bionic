#include "regs_ins8070.h"
#include "debugger.h"
#include "inst_ins8070.h"
#include "pins_ins8070.h"

namespace debugger {
namespace ins8070 {
namespace {
const char line[] =
        "PC=xxxx SP=xxxx P2=xxxx P3=xxxx E=xx A=xx T=xxxx S=CVBA321I";
//       01234567890123456789012345678901234567890123456789012345678
}  // namespace

RegsIns8070::RegsIns8070(PinsIns8070 *pins) : _pins(pins), _buffer(line) {}

void RegsIns8070::print() const {
    _buffer.hex16(3, _pc());
    _buffer.hex16(11, _sp());
    _buffer.hex16(19, _p2());
    _buffer.hex16(27, _p3());
    _buffer.hex8(34, _e);
    _buffer.hex8(39, _a);
    _buffer.hex16(44, _t);
    _buffer.bits(51, _s, 0x80, line + 51);
    cli.println(_buffer);
    _pins->idle();
}

void RegsIns8070::save() {
    // The first write's address is the saved PC. S is stored before IE is
    // cleared; SP moves off internal RAM so the pushes show on the bus.
    static constexpr uint8_t PUSH_ALL[] = {
            0x88, 0xFE,        // ST EA,-1,PC
            0x06, 0xC8, 0xFE,  // LD A,S; ST A,-1,PC
            0x39, 0xFE,        // AND S,=0xFE; clear IE
            0x31, 0x88, 0xFE,  // LD EA,SP; ST EA,-1,PC
            0x25, 0x00, 0x01,  // LD SP,=0x0100
            0x56,              // PUSH P2
            0x57,              // PUSH P3
            0x0B, 0x08,        // LD EA,T; PUSH EA
    };
    uint8_t buffer[11];
    _pins->captureWrites(
            PUSH_ALL, sizeof(PUSH_ALL), &_pc(), buffer, sizeof(buffer));
    _a = buffer[0];
    _e = buffer[1];
    _s = buffer[2];
    _sp() = le16(buffer + 3);
    _p2() = be16(buffer + 5);
    _p3() = be16(buffer + 7);
    _t = be16(buffer + 9);
}

void RegsIns8070::restore() {
    const uint8_t LD_ALL[] = {
            0x27, lo(_p3()), hi(_p3()),  // LD P3,=_p3
            0x26, lo(_p2()), hi(_p2()),  // LD P2,=_p2
            0x25, lo(_sp()), hi(_sp()),  // LD SP,=_sp
            0xA4, lo(_t), hi(_t),        // LD T,=_t
            0x84, _s, _e,                // LD EA,=_e|_s
            0x07,                        // LD S,A
            0xC4, _a,                    // LD A,=_a
            0x24, lo(_pc()), hi(_pc()),  // JMP =_pc
    };
    _pins->execInst(LD_ALL, sizeof(LD_ALL));
}

uint16_t RegsIns8070::effectiveAddr(
        const InstIns8070 &inst, uint8_t opr) const {
    const auto disp = static_cast<int8_t>(opr);
    const auto base = _ptr[inst.opc & 3];
    switch (inst.addrMode()) {
    case M_DISP:
        return base + disp;
    case M_PUSH:
        return _sp() - 1;
    case M_POP:
        return _sp();
    case M_AUTO:
        // Pre-decrement for a negative disp, post-increment otherwise.
        return disp < 0 ? base + disp : base;
    case M_DIR:
        return 0xff00 + opr;
    case M_SSM:
        return base;
    default:
        return 0;
    }
}

void RegsIns8070::helpRegisters() const {
    cli.println("?Reg: PC SP P2 P3 A E EA T S");
}

constexpr const char *REGS8[] = {
        "A",  // 1
        "E",  // 2
        "S",  // 3
};
constexpr const char *REGS16[] = {
        "EA",  // 4
        "T",   // 5
        "PC",  // 6
        "SP",  // 7
        "P2",  // 8
        "P3",  // 9
};

const Regs::RegList *RegsIns8070::listRegisters(uint_fast8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS8, 3, 1, UINT8_MAX},
            {REGS16, 6, 4, UINT16_MAX},
    };
    return n < 2 ? &REG_LIST[n] : nullptr;
}

bool RegsIns8070::setRegister(uint_fast8_t reg, uint32_t value) {
    switch (reg) {
    default:
        _ptr[reg - 6] = value;
        return reg == 6;
    case 1:
        _a = value;
        break;
    case 2:
        _e = value;
        break;
    case 4:
        _ea(value);
        break;
    case 5:
        _t = value;
        break;
    case 3:
        _s = value;
        break;
    }
    return false;
}

uint8_t RegsIns8070::internal_read(uint16_t addr) const {
    // Internal RAM cycles show nothing on the bus; store it out through A.
    uint8_t LD_ST[] = {
            0xC5, uint8(addr),  // LD A,dir[addr]
            0xC8, 0xFE          // ST A,-1,PC
    };
    uint8_t data;
    _pins->captureWrites(LD_ST, sizeof(LD_ST), nullptr, &data, sizeof(data));
    return data;
}

void RegsIns8070::internal_write(uint16_t addr, uint8_t data) const {
    // Internal RAM cycles show nothing on the bus; write it through A.
    const uint8_t LD_ST[] = {
            0xC4, data,        // LD A,data
            0xCD, uint8(addr)  // ST A,dir[addr]
    };
    _pins->execInst(LD_ST, sizeof(LD_ST));
}

}  // namespace ins8070
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
