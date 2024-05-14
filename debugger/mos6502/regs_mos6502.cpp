#include "regs_mos6502.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "inst_mos6502.h"
#include "mems_w65c816.h"
#include "pins_mos6502.h"
#include "signals_mos6502.h"
#include "target_mos6502.h"

namespace debugger {
namespace mos6502 {

struct RegsMos6502 Registers;

const char *RegsMos6502::cpu() const {
    static constexpr const char *CPU_NAMES[/*SoftwareType*/] = {
            "MOS6502",
            "G65SC02",
            "R65C02",
            "W65C02S",
            "W65C816S",
    };
    const auto type = Pins.softwareType();
    return CPU_NAMES[type];
}

const char *RegsMos6502::cpuName() const {
    return cpu();
}

void RegsMos6502::print() const {
    if (Pins.native65816()) {
        // clang-format off
        //                              012345678901234567890123456789012345678901234567890123456789012
        static constexpr char line[] = "K=xx PC=xxxx S=xxxx D=xxxx B=xx X=xxxx Y=xxxx C=xxxx P=NVMXDIZC";
        // clang-format on
        static auto &buffer = *new CharBuffer(line);
        buffer.hex8(2, _pbr);
        buffer.hex16(8, _pc);
        buffer.hex16(15, _s);
        buffer.hex16(22, _d);
        buffer.hex8(29, _dbr);
        buffer.hex16(34, _x);
        buffer.hex16(41, _y);
        buffer.hex16(48, _c());
        buffer.bits(55, _p, 0x80, line + 55);
        cli.println(buffer);
    } else {
        // clang-format off
        //                              0123456789012345678901234567890123456789
        static constexpr char line[] = "PC=xxxx S=xxxx X=xx Y=xx A=xx P=NV_BDIZC E=1";
        // clang-format on
        static auto &buffer = *new CharBuffer(line);
        buffer.hex16(3, _pc);
        buffer.hex16(10, _s);
        buffer.hex8(17, _x);
        buffer.hex8(22, _y);
        buffer.hex8(27, _a);
        buffer.bits(32, _p, 0x80, line + 32);
        buffer[40] = Pins.hardwareType() == HW_W65C816 ? ' ' : 0;
        cli.println(buffer);
    }
}

static constexpr uint8_t noop = 0xFF;

void RegsMos6502::save() {
    if (Pins.native65816()) {
        save65816();
        return;
    }

    static const uint8_t PUSH_ALL[] = {
            // 6502:  JSR PCL --- PCH
            // 65816: JSR PCL PCH ---
            0x20, 0x00, 0x02, 0x02,  // JSR $0200
            0x08, noop,              // PHP
            0x48, noop,              // PHA
            0x8A, noop,              // TXA
            0x48, noop,              // PHA
            0x98, noop,              // TYA
            0x48, noop,              // PHA
    };
    uint8_t buffer[6];

    uint16_t sp;
    Pins.captureWrites(PUSH_ALL, sizeof(PUSH_ALL), &sp, buffer, sizeof(buffer));

    _pc = be16(buffer) - 2;  // pc on stack points the last byte of JSR.
    _s = lo(sp) | 0x0100;
    _e = 1;
    setP(buffer[2]);
    _a = buffer[3];
    _x = buffer[4];
    _y = buffer[5];
}

void RegsMos6502::save65816() {
    static const uint8_t PUSH_ALL[] = {
            0x22, 0x00, 0x02, noop, 0x00,  // JSL $000200; PBR=0, PC=1
            0x08, noop,                    // PHP; P=3
            0xC2, 0x30, noop,              // REP #$30; M=0, X=0
            0x48, noop,                    // PHA; C=4
            0x8B, noop,                    // PHB; DBR=6
            0xDA, noop,                    // PHX; X=7
            0x5A, noop,                    // PHY; Y=9
            0x0B, noop,                    // PHD; D=11
    };
    uint8_t buffer[13];

    Pins.captureWrites(PUSH_ALL, sizeof(PUSH_ALL), &_s, buffer, sizeof(buffer));

    _pbr = buffer[0];
    _pc = be16(buffer + 1) - 3;  // pc on stack points the last byte of JSL.
    _e = 0;
    setP(buffer[3]);
    _b = buffer[4];
    _a = buffer[5];
    _dbr = buffer[6];
    _x = be16(buffer + 7);
    _y = be16(buffer + 9);
    _d = be16(buffer + 11);
}

void RegsMos6502::setE(uint8_t value) {
    static uint8_t SET_E[] = {
            0x38, noop,  // SEC/CLC
            0xFB, noop,  // XCE
    };
    SET_E[0] = value ? InstMos6502::SEC : InstMos6502::CLC;
    Pins.execInst(SET_E, sizeof(SET_E));
}

void RegsMos6502::restore() {
    if (_e == 0) {
        restore65816();
        return;
    }

    if (Pins.hardwareType() == HW_W65C816)
        setE(_e);

    static uint8_t PULL_ALL[] = {
            0xA2, 0,     // s:1 LDX #s
            0x9A, noop,  // TXS
            0xA0, 0,     // y:5 LDY #y
            0xA2, 0,     // x:7 LDX #x
            0xA9, 0,     // a:9 LDA #a
            0x40,        // RTI
            noop,        // fetch next op code
            noop,        // discarded stack fetch
            0,           // p:13
            0,           // lo(pc):14
            0,           // hi(pc):15
    };

    PULL_ALL[1] = _s - 3;  // offset for RTI
    PULL_ALL[5] = _y;
    PULL_ALL[7] = _x;
    PULL_ALL[9] = _a;
    PULL_ALL[13] = _p;
    setle16(PULL_ALL + 14, _pc);

    Pins.execInst(PULL_ALL, sizeof(PULL_ALL));
}

void RegsMos6502::restore65816() {
    static uint8_t PULL_ALL[] = {
            0xC2, 0x30, noop,     //        REP #$30; M=0, X=0
            0xA9, 0, 0,           // d:4    LDA #d
            0x5B, noop,           //        TCD
            0xA0, 0, 0,           // y:9    LDY #y
            0xA2, 0, 0,           // x:12   LDX #x
            0xA9, 0, 0,           // s:15   LDA #s-4
            0x1B, noop,           //        TCS
            0xA9, 0, 0,           // c:20   LDA #c
            0xAB, noop, noop, 0,  // dbr:25 PLB (dbr)
            0x40,                 //        RTI
            noop, noop,           //        fetch next op and increment stack
            0,                    // p:29
            0, 0,                 // pc:30
            0,                    // pbr:32
    };

    setE(_e);
    setle16(PULL_ALL + 4, _d);
    setle16(PULL_ALL + 9, _y);
    setle16(PULL_ALL + 12, _x);
    setle16(PULL_ALL + 15, _s - 5);  // offset PLB(dbr) and RTI(p,pc,pbr)
    PULL_ALL[20] = _a;
    PULL_ALL[21] = _b;
    PULL_ALL[25] = _dbr;
    PULL_ALL[29] = _p;
    setle16(PULL_ALL + 30, _pc);
    PULL_ALL[32] = _pbr;

    Pins.execInst(PULL_ALL, sizeof(PULL_ALL));
}

void RegsMos6502::helpRegisters() const {
    cli.print(F("?Reg: PC S X Y A P"));
    if (Pins.hardwareType() == HW_W65C816) {
        cli.print(F(" E"));
        if (_e == 0)
            cli.print(F(" C D K/PBR B/DBR PM PX"));
    }
    cli.println();
}

constexpr const char *REGS8[] = {
        "A",  // 1
        "P",  // 2
};
constexpr const char *REGS16[] = {
        "PC",  // 3
};
constexpr const char *REGS8_6502[] = {
        "X",  // 4
        "Y",  // 5
        "S",  // 6
};
constexpr const char *REGS16_65816[] = {
        "X",  // 4
        "Y",  // 5
        "S",  // 6
        "C",  // 7
        "D",  // 8
};
constexpr const char *REGS8_65816[] = {
        "B",    // 9
        "DBR",  // 10
        "K",    // 11
        "PBR",  // 12
};
constexpr const char *REGS1_65816[] = {
        "E",   // 13
        "PM",  // 14
        "PX",  // 15
};

const Regs::RegList *RegsMos6502::listRegisters(uint8_t n) const {
    static constexpr RegList REG_LIST_6502[] = {
            {REGS8, 2, 1, UINT8_MAX},
            {REGS16, 1, 3, UINT16_MAX},
            {REGS8_6502, 3, 4, UINT8_MAX},
            {REGS1_65816, 1, 13, 1},
    };
    static constexpr RegList REG_LIST_65816[] = {
            {REGS8, 2, 1, UINT8_MAX},
            {REGS16, 1, 3, UINT16_MAX},
            {REGS16_65816, 5, 4, UINT16_MAX},
            {REGS8_65816, 4, 9, UINT8_MAX},
            {REGS1_65816, 3, 13, 1},
    };
    if (_e == 0) {
        return n < 5 ? &REG_LIST_65816[n] : nullptr;
    } else {
        const auto num = Pins.hardwareType() == HW_W65C816 ? 4 : 3;
        return n < num ? &REG_LIST_6502[n] : nullptr;
    }
}

void RegsMos6502::setP(uint8_t value) {
    if (_e == 0) {
        _p = value;
        w65c816::Memory.longA((_p & P_M) == 0);
        w65c816::Memory.longI((_p & P_X) == 0);
    } else {
        w65c816::Memory.longA(false);
        w65c816::Memory.longI(false);
        _p = value | 0x20;
    }
}

void RegsMos6502::setRegister(uint8_t reg, uint32_t value) {
    switch (reg) {
    case 3:
        _pc = value;
        break;
    case 6:
        if (_e == 0) {
            _s = value;
        } else {
            _s = value | 0x0100;
        }
        break;
    case 4:
        _x = value;
        break;
    case 5:
        _y = value;
        break;
    case 8:
        _d = value;
        break;
    case 1:
        _a = value;
        break;
    case 7:
        _c(value);
        break;
    case 2:
        setP(value);
        break;
    case 9:
    case 10:
        _dbr = value;
        break;
    case 11:
    case 12:
        _pbr = value;
        break;
    case 13:
        setE(_e = value);
        break;
    case 14:
        setP(value ? (_p | P_M) : (_p & ~P_M));
        break;
    case 15:
        setP(value ? (_p | P_X) : (_p & ~P_X));
        break;
    }
}
}  // namespace mos6502
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
