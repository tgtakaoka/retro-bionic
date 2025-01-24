#include "regs_cdp1802.h"
#include "debugger.h"
#include "inst_cdp1802.h"
#include "pins_cdp1802.h"
#include "signals_cdp1802.h"

namespace debugger {
namespace cdp1802 {

namespace {

const char CDP1802[] = "CDP1802";
const char CDP1804A[] = "CDP1804A";
// clang-format off
//                              1         2         3         4         5         6         7
//                    01234567890123456789012345678901234567890123456789012345678901234567890
const char line1[] = "D=xx DF=x X=x P=x T=xx Q=x IE=1";
const char line2[] = "R0=xxxx  R1=xxxx  R2=xxxx  R3=xxxx  R4=xxxx  R5=xxxx  R6=xxxx  R7=xxxx";
const char line3[] = "R8=xxxx  R9=xxxx R10=xxxx R11=xxxx R12=xxxx R13=xxxx R14=xxxx R15=xxxx";
// clang-format on

}  // namespace

RegsCdp1802::RegsCdp1802(PinsCdp1802 *pins)
    : _pins(pins), _buffer1(line1), _buffer2(line2), _buffer3(line3) {}

/**
 * - CDP1802: 0x68 opcode causes illegal write on M(R(P)).
 * - CDP1804/CDP1804A:0x68 opcode causes double instruction fetch.
 * TODO: distinguish CDP1804 and CDP1804A
 */
void RegsCdp1802::setCpuType() {
    _pins->inject(InstCdp1802::PREFIX);
    auto s = _pins->prepareCycle();
    if (s->fetch()) {
        _cpuType = CDP1804A;
        _pins->inject(InstCdp1802::DADI);
        _pins->inject(0);
        _pins->cycle();  // execution cycle
    } else {
        _cpuType = CDP1802;
        s->capture();  // capture illegal write
        _pins->cycle();
    }
}

const char *RegsCdp1802::cpu() const {
    return _cpuType ? _cpuType : CDP1802;
}

const char *RegsCdp1802::cpuName() const {
    return cpu();
}

void RegsCdp1802::reset() {
    _cpuType = nullptr;
}

bool RegsCdp1802::is1804() const {
    return _cpuType == CDP1804A;
}

void RegsCdp1802::print() const {
    _buffer1.hex8(2, _d);
    _buffer1.hex4(8, _df);
    _buffer1.hex4(12, _x);
    _buffer1.hex4(16, _p);
    _buffer1.hex8(20, _t);
    _buffer1.hex4(25, _q);
    _buffer1.hex4(30, _ie);
    cli.println(_buffer1);
    for (auto i = 0; i < 8; i++)
        _buffer2.hex16(3 + i * 9, _r[i]);
    cli.println(_buffer2);
    for (auto i = 0; i < 8; i++)
        _buffer3.hex16(3 + i * 9, _r[i + 8]);
    cli.println(_buffer3);
}

void RegsCdp1802::save() {
    static constexpr uint8_t SAV[] = {0x78};
    _pins->captureWrites(SAV, sizeof(SAV), nullptr, &_t, sizeof(_t));

    // STR R0, STR R1, MARK, STR R3, ...
    static constexpr uint8_t STR[] = {
            0x50, 0x51, 0x79, 0x53, 0x54, 0x55, 0x56, 0x57,  //
            0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,  //
    };
    for (auto i = 0; i < 16; i++) {
        uint8_t tmp;
        _pins->captureWrites(&STR[i], 1, &_r[i], &tmp, sizeof(tmp));
        if (i == 0)
            _d = tmp;
        if (i == 2) {  // MARK
            _x = hi4(tmp);
            _p = lo4(tmp);
        }
        _dirty[i] = false;
    }
    _dirty[2] = true;                // becase of MARK
    _dirty[_p] = true;               // becase this is a program counter
    _r[_p] -= sizeof(SAV) + _p + 1;  // adjust program counter

    _df = _pins->skip(InstCdp1802::LSDF);  // LSDF: skip if DF=1
    _q = _pins->skip(InstCdp1802::LSQ);    // LSQ: skip if Q=1
    _ie = _pins->skip(InstCdp1802::LSIE);  // LSIE: skip if IE=1
    if (_cpuType == nullptr)
        setCpuType();
}

void RegsCdp1802::restore() {
    const uint8_t p = lo4(_t);
    const uint8_t x = hi4(_t);
    _dirty[p] = true;
    const uint8_t LDT[] = {
            uint8(InstCdp1802::SEP | p),  // SEP Rn
            uint8(InstCdp1802::SEX | x),  // SEX Rn
            0x79,                         // MARK
    };
    uint8_t tmp;
    _pins->captureWrites(LDT, sizeof(LDT), nullptr, &tmp, sizeof(tmp));

    const auto q = _q ? InstCdp1802::SEQ : InstCdp1802::REQ;
    const uint8_t LDQ_DF[] = {
            q,                         // REQ:0x7A, SEQ:0x7B
            0xF8, uint8(_df ? 1 : 0),  // LDI df
            0x76,                      // SHRC
    };
    _pins->execInst(LDQ_DF, sizeof(LDQ_DF));

    static const uint8_t SEP15_SEX14[] = {
            0xDF,  // SEP R15
            0xEE   // SEX R14
    };
    _pins->execInst(SEP15_SEX14, sizeof(SEP15_SEX14));
    _dirty[14] = _dirty[15] = true;

    const uint8_t LDD[] = {
            0xF8, _d  // LDI d
    };
    for (auto i = 0; i < 16; i++) {
        if (_dirty[i]) {
            auto rn = _r[i];
            const uint8_t LDR[] = {
                    0xF8, hi(rn),     // LDI hi(Rn)
                    uint8(0xB0 | i),  // PHI Rn
                    0xF8, lo(rn),     // LDI lo(Rn)
                    uint8(0xA0 | i),  // PLO Rn
            };
            if (i == 14)
                rn -= 1;  // offset R14
            if (i == 15)
                rn -= sizeof(LDD) + 1;  // offset R15
            _pins->execInst(LDR, sizeof(LDR));
        }
    }
    _pins->execInst(LDD, sizeof(LDD));  // R15+=2

    const auto ie = _ie ? InstCdp1802::RET : InstCdp1802::DIS;
    const uint8_t RET[] = {
            ie,             // RET: 0x70 or DIS:0x71
            uint8(_x, _p),  // x,p
    };
    _pins->execInst(RET, sizeof(RET));  // R15++, R14++
}

void RegsCdp1802::helpRegisters() const {
    cli.println("?Reg: D DF X P T IE Q R0~R15 RP RX");
}

constexpr const char *REGS1[] = {
        "DF",  // 1
        "IE",  // 2
        "Q",   // 3
};
constexpr const char *REGS4[] = {
        "P",  // 4
        "X",  // 5
};
constexpr const char *REGS8[] = {
        "D",  // 6
        "T",  // 7
};
constexpr const char *REGS16[] = {
        "R0",   // 8
        "R1",   // 9
        "R2",   // 10
        "R3",   // 11
        "R4",   // 12
        "R5",   // 13
        "R6",   // 14
        "R7",   // 15
        "R8",   // 16
        "R9",   // 17
        "R10",  // 18
        "R11",  // 19
        "R12",  // 20
        "R13",  // 21
        "R14",  // 22
        "R15",  // 23
        "RP",   // 24
        "RX",   // 25
};

const Regs::RegList *RegsCdp1802::listRegisters(uint_fast8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS1, 3, 1, 1},
            {REGS4, 2, 4, 0xF},
            {REGS8, 2, 6, UINT8_MAX},
            {REGS16, 18, 8, UINT16_MAX},
    };
    return n < 4 ? &REG_LIST[n] : nullptr;
}

bool RegsCdp1802::setRegister(uint_fast8_t reg, uint32_t value) {
    switch (reg) {
    case 1:
        _df = value;
        break;
    case 6:
        _d = value;
        break;
    case 4:
        _p = value;
        return true;
    case 5:
        _x = value;
        break;
    case 7:
        _t = value;
        break;
    case 2:
        _ie = value;
        break;
    case 3:
        _q = value;
        break;
    case 24:
        _r[_p] = value;
        _dirty[_p] = true;
        return true;
    case 25:
        _r[_x] = value;
        _dirty[_x] = true;
        break;
    default:
        _r[reg - 8] = value;
        _dirty[reg - 8] = true;
        return reg - 8 == _p;
    }
    return false;
}

}  // namespace cdp1802
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
