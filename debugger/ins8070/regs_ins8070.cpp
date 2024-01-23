#include "regs_ins8070.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "inst_ins8070.h"
#include "mems_ins8070.h"
#include "pins_ins8070.h"
#include "signals_ins8070.h"

namespace debugger {
namespace ins8070 {

struct RegsIns8070 Regs;

const char *RegsIns8070::cpu() const {
    return "INS8070";
}

const char *RegsIns8070::cpuName() const {
    return cpu();
}

void RegsIns8070::print() const {
    static constexpr char line[] =
            "PC=xxxx SP=xxxx P2=xxxx P3=xxxx E=xx A=xx T=xxxx S=CVBA321I";
    //       01234567890123456789012345678901234567890123456789012345678
    static auto &buffer = *new CharBuffer(line);
    buffer.hex16(3, _pc());
    buffer.hex16(11, _sp());
    buffer.hex16(19, _p2());
    buffer.hex16(27, _p3());
    buffer.hex8(34, _e);
    buffer.hex8(39, _a);
    buffer.hex16(44, _t);
    buffer.bits(51, _s, 0x80, &line[51]);
    cli.println(buffer);
    Pins.idle();
}

void RegsIns8070::save() {
    static const uint8_t PUSH_ALL[] = {
            0x88, 0xFE,        // ST EA,-1,PC
            0x31, 0x88, 0xFE,  // LD EA,SP; ST EA,-1,PC
            0x25, 0x00, 0x01,  // LD SP,=0x0100
            0x56,              // PUSH P2
            0x57,              // PUSH P3
            0x0B, 0x08,        // LD EA,T; PUSH EA
            0x06, 0x0A,        // LD A,S; PUSH A
    };
    static uint8_t buffer[11];
    Pins.captureWrites(
            PUSH_ALL, sizeof(PUSH_ALL), &_pc(), buffer, sizeof(buffer));
    _a = buffer[0];
    _e = buffer[1];
    _sp() = le16(buffer + 2);
    _p2() = be16(buffer + 4);
    _p3() = be16(buffer + 6);
    _t = be16(buffer + 8);
    _s = buffer[10];
}

void RegsIns8070::restore() {
    static uint8_t LD_ALL[] = {
            0x27, 0, 0,  // p3=2:1; LD P3,=p3
            0x26, 0, 0,  // p2=5:4; LD P2,=p2
            0x25, 0, 0,  // sp=8:7; LD SP,=sp
            0xA4, 0, 0,  // t=11:10; LD T,=t
            0x84, 0, 0,  // e=14 s=13; LD EA,=e|s
            0x07,        // LD S,A
            0xC4, 0,     // a=17; LD A,=a
            0x24, 0, 0,  // pc=20:19; JMP =pc
    };

    LD_ALL[1] = lo(_p3());
    LD_ALL[2] = hi(_p3());
    LD_ALL[4] = lo(_p2());
    LD_ALL[5] = hi(_p2());
    LD_ALL[7] = lo(_sp());
    LD_ALL[8] = hi(_sp());
    LD_ALL[10] = lo(_t);
    LD_ALL[11] = hi(_t);
    LD_ALL[13] = _s;
    LD_ALL[14] = _e;
    LD_ALL[17] = _a;
    LD_ALL[19] = lo(_pc());
    LD_ALL[20] = hi(_pc());
    Pins.execInst(LD_ALL, sizeof(LD_ALL));
}

uint8_t RegsIns8070::busCycles(InstIns8070 &inst) const {
    if (!inst.get(nextIp()))
        return 0;
    const auto ea = effectiveAddr(inst);
    return MemsIns8070::is_internal(ea) ? inst.externalCycles()
                                        : inst.busCycles();
}

uint16_t RegsIns8070::effectiveAddr(const InstIns8070 &inst) const {
    const auto opr = Memory.raw_read(inst.addr + 1);
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
        return disp < 0 ? base - disp : base;
    case M_DIR:
        return 0xff00 + opr;
    case M_SSM:
        return base;
    default:
        return 0;
    }
}

void RegsIns8070::helpRegisters() const {
    cli.println(F("?Reg: PC SP P2 P3 A E EA T S"));
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

const Regs::RegList *RegsIns8070::listRegisters(uint8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS8, 3, 1, UINT8_MAX},
            {REGS16, 6, 4, UINT16_MAX},
    };
    return n < 2 ? &REG_LIST[n] : nullptr;
}

void RegsIns8070::setRegister(uint8_t reg, uint32_t value) {
    switch (reg) {
    default:
        _ptr[reg - 6U] = value;
        break;
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
}

uint8_t RegsIns8070::internal_read(uint16_t addr) {
    // No bus signals while internal RAM bus cycle.
    static uint8_t LD_ST[] = {
            0xC5, 0,    // LD A,dir[addr]
            0xCD, 0x00  // ST A,dir[0x00]
    };
    LD_ST[1] = addr;
    uint8_t data;
    Pins.captureWrites(LD_ST, sizeof(LD_ST), nullptr, &data, sizeof(data));
    return data;
}

void RegsIns8070::internal_write(uint16_t addr, uint8_t data) {
    // No bus signals while internal RAM bus cycle.
    static uint8_t LD_ST[] = {
            0xC4, 0,  // LD A,data
            0xCD, 0   // ST A,dir[addr]
    };
    LD_ST[1] = data;
    LD_ST[3] = addr;
    Pins.execInst(LD_ST, sizeof(LD_ST));
}

}  // namespace ins8070
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
