#include "regs_mos6502.h"
#include "debugger.h"
#include "inst_mos6502.h"
#include "mems_mos6502.h"
#include "pins_mos6502.h"

namespace debugger {
namespace mos6502 {
namespace {
const char *const CPU_NAMES[/*SoftwareType*/] = {
        "MOS6502",
        "G65SC02",
        "R65C02",
        "W65C02S",
        "W65C816S",
};

// clang-format off
//                                  1         2         3         4         5         6
//                        012345678901234567890123456789012345678901234567890123456789012
const char line6502[]  = "PC=xxxx S=xxxx X=xx Y=xx A=xx P=NV_BDIZC E=1";
const char line65816[] = "K:PC=xx:xxxx S=xxxx D=xxxx B=xx X=xxxx Y=xxxx C=xxxx P=NVMXDIZC";
// clang-format on
}  // namespace

RegsMos6502::RegsMos6502(PinsMos6502 *pins, MemsMos6502 *mems)
    : _pins(pins), _mems(mems), _buffer(line6502) {}

const char *RegsMos6502::cpu() const {
    const auto type = PinsMos6502::softwareType();
    return CPU_NAMES[type];
}

uint32_t RegsMos6502::nextIp() const {
    if (_pins->native65816()) {
        return (static_cast<uint32_t>(_pbr) << 16) | _pc;
    } else {
        return _pc;
    }
}

void RegsMos6502::setIp(uint32_t pc) {
    _pc = static_cast<uint16_t>(pc);
    if (_pins->native65816())
        _pbr = pc >> 16;
}

void RegsMos6502::print() const {
    if (_pins->native65816()) {
        _buffer.set(line65816);
        _buffer.hex8(5, _pbr);
        _buffer.hex16(8, _pc);
        _buffer.hex16(15, _s);
        _buffer.hex16(22, _d);
        _buffer.hex8(29, _dbr);
        _buffer.hex16(34, _x);
        _buffer.hex16(41, _y);
        _buffer.hex16(48, _c());
        _buffer.bits(55, _p, 0x80, line65816 + 55);
    } else {
        _buffer.set(line6502);
        _buffer.hex16(3, _pc);
        _buffer.hex16(10, _s);
        _buffer.hex8(17, _x);
        _buffer.hex8(22, _y);
        _buffer.hex8(27, _a);
        _buffer.bits(32, _p, 0x80, line6502 + 32);
        _buffer[40] = PinsMos6502::hardwareType() == HW_W65C816 ? ' ' : 0;
    }
    cli.println(_buffer);
}

static constexpr uint8_t noop = 0xFF;

void RegsMos6502::save() {
    if (_pins->native65816()) {
        save65816();
        return;
    }

    static constexpr uint8_t PUSH_ALL[] = {
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
    _pins->captureWrites(
            PUSH_ALL, sizeof(PUSH_ALL), &sp, buffer, sizeof(buffer));

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

    _pins->captureWrites(
            PUSH_ALL, sizeof(PUSH_ALL), &_s, buffer, sizeof(buffer));

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
    static constexpr uint8_t SET[] = {
            InstMos6502::SEC, noop,  // SEC
            0xFB, noop,              // XCE
    };
    static constexpr uint8_t CLR[] = {
            InstMos6502::CLC, noop,  // CLC
            0xFB, noop,              // XCE
    };
    _pins->execInst(value ? SET : CLR, sizeof(SET));
}

void RegsMos6502::restore() {
    if (_e == 0) {
        restore65816();
        return;
    }

    if (PinsMos6502::hardwareType() == HW_W65C816)
        setE(_e);

    const auto sp = _s - 3;  // offset RTI
    // clang-format off
    const uint8_t PULL_ALL[] = {
            0xA2, uint8(sp),  // LDX #sp
            0x9A, noop,       // TXS
            0xA0, uint8(_y),  // LDY #_y
            0xA2, uint8(_x),  // LDX #_x
            0xA9, uint8(_a),  // LDA #_a
            0x40,             // RTI
            noop,             // fetch next op code
            noop,             // discarded stack fetch
            _p, lo(_pc), hi(_pc),
    };
    // clang-format on
    _pins->execInst(PULL_ALL, sizeof(PULL_ALL));
}

void RegsMos6502::restore65816() {
    const auto sp = _s - 5;  // offset PLB(dbr) and RTI(p,pc,pbr)
    const uint8_t PULL_ALL[] = {
            0xC2, 0x30, noop,        // REP #$30; M=0, X=0
            0xA9, lo(_d), hi(_d),    // LDA #_d
            0x5B, noop,              // TCD
            0xA0, lo(_y), hi(_y),    // LDY #y
            0xA2, lo(_x), hi(_x),    // LDX #x
            0xA9, lo(sp), hi(sp),    // LDA #sp
            0x1B, noop,              // TCS
            0xA9, _a, _b,            // LDA #_b|_a
            0xAB, noop, noop, _dbr,  // PLB (_dbr)
            0x40,                    // RTI
            noop, noop,              // fetch next op and increment stack
            _p,                      // P
            lo(_pc), hi(_pc),        // PC
            _pbr,                    // PBR
    };

    setE(_e);
    _pins->execInst(PULL_ALL, sizeof(PULL_ALL));
}

void RegsMos6502::helpRegisters() const {
    cli.print("?Reg: PC S X Y A P");
    if (PinsMos6502::hardwareType() == HW_W65C816) {
        cli.print(" E");
        if (_e == 0)
            cli.print(" C D K/PBR B/DBR PM PX");
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

const Regs::RegList *RegsMos6502::listRegisters(uint_fast8_t n) const {
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
        const uint_fast8_t num =
                PinsMos6502::hardwareType() == HW_W65C816 ? 4 : 3;
        return n < num ? &REG_LIST_6502[n] : nullptr;
    }
}

void RegsMos6502::setP(uint8_t value) {
    if (_e == 0) {
        _p = value;
        _mems->longA((_p & P_M) == 0);
        _mems->longI((_p & P_X) == 0);
    } else {
        _mems->longA(false);
        _mems->longI(false);
        _p = value | 0x20;
    }
}

bool RegsMos6502::setRegister(uint_fast8_t reg, uint32_t value) {
    switch (reg) {
    case 3:
        _pc = value;
        return true;
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
        return true;
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
    return false;
}
}  // namespace mos6502
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
