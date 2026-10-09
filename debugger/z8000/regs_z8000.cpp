#include "regs_z8000.h"

#include "debugger.h"
#include "inst_z8000.h"
#include "pins_z8000.h"

namespace debugger {
namespace z8000 {
namespace {
// clang-format off
//                              1         2         3         4         5         6         7
//                    01234567890123456789012345678901234567890123456789012345678901234567890
const char line1[] = "PC=xxxx NSP=xxxx FCW=SEVN CZSPDH  PSAP=xxxx";
const char line2[] = "R0=xxxx  R1=xxxx  R2=xxxx  R3=xxxx  R4=xxxx  R5=xxxx  R6=xxxx  R7=xxxx";
const char line3[] = "R8=xxxx  R9=xxxx R10=xxxx R11=xxxx R12=xxxx R13=xxxx R14=xxxx R15=xxxx";
// clang-format on

// Where the stores of the save sequences go: they are captured and
// never reach memory.
constexpr uint16_t STORE = 0xF000;
// A trap pushes the PC, the FCW and an identifier (Z8002).
constexpr uint16_t FRAME_BYTES = 6;
}  // namespace

RegsZ8000::RegsZ8000(PinsZ8000 *pins)
    : _pins(pins), _buffer1(line1), _buffer2(line2), _buffer3(line3) {}

bool RegsZ8000::normalMode() const {
    return (_fcw & InstZ8000::FCW_SN) == 0;
}

void RegsZ8000::setR15(uint16_t v) {
    if (normalMode()) {
        _nsp = v;
    } else {
        _r[15] = v;
    }
}

void RegsZ8000::print() const {
    auto status = _buffer1;
    // The stack pointer the program can't see.
    if (normalMode()) {
        status[8] = 'S';
        status[9] = 'S';
    }
    status.hex16(3, _pc);
    status.hex16(12, normalMode() ? _r[15] : _nsp);
    status.bits(21, _fcw, 0x4000, "SEVN");
    status.bits(26, _fcw, 0x0080, "CZSPDH");
    status.hex16(39, _psap);
    cli.println(status);
    _pins->idle();
    auto low = _buffer2;
    auto high = _buffer3;
    for (auto i = 0; i < 8; ++i) {
        low.hex16(3 + 9 * i, _r[i]);
        high.hex16(3 + 9 * i, _r[8 + i]);
    }
    if (normalMode())
        high.hex16(3 + 9 * 7, _nsp);
    cli.println(low);
    _pins->idle();
    cli.println(high);
    _pins->idle();
}

void RegsZ8000::save() {
    // Each sequence ends with a JR $+2: the CPU fetches ahead, so the
    // exit is fetched again after the stores.
    // clang-format off
    static constexpr uint8_t SAVE_ALL[] = {
        0x5C, 0x09, 0x00, 0x0F, hi(STORE), lo(STORE),  // LDM STORE, R0, #16
        0xE8, 0x00,                                    // JR $+2
    };
    static constexpr uint8_t SAVE_CONTROL[] = {
        0x7D, 0x07,                                    // LDCTL R0, NSP
        0x7D, 0x15,                                    // LDCTL R1, PSAP
        0x5C, 0x09, 0x00, 0x01, hi(STORE), lo(STORE),  // LDM STORE, R0, #2
        0xE8, 0x00,                                    // JR $+2
    };
    // clang-format on
    const auto parked = _parkedAt;
    auto org = parked;
    uint8_t buffer[32];
    _pins->captureWrites(SAVE_ALL, sizeof(SAVE_ALL), buffer, 32, org);
    for (auto i = 0; i < 16; ++i)
        _r[i] = be16(buffer + 2 * i);
    _pins->captureWrites(SAVE_CONTROL, sizeof(SAVE_CONTROL), buffer, 4, org);
    _nsp = be16(buffer + 0);
    _psap = be16(buffer + 2) & 0xFF00;  // the offset's low byte is 0
    // Back where the CPU was parked, with the two registers that
    // SAVE_CONTROL used as they were.
    // clang-format off
    const uint8_t BACK[] = {
        0x21, 0x00, hi(_r[0]), lo(_r[0]),  // LD R0, #r0
        0x21, 0x01, hi(_r[1]), lo(_r[1]),  // LD R1, #r1
        0x5E, 0x08, hi(parked), lo(parked),  // JP parked
    };
    // clang-format on
    _pins->execInst(BACK, sizeof(BACK), org, parked);
    _parkedAt = org;
    // The program's SSP is above the frame the debugger's trap pushed.
    if (_frame) {
        _r[15] += FRAME_BYTES;
        _frame = false;
    }
}

void RegsZ8000::restore() {
    // One window: the control registers, every general register from the
    // block LDM reads, then LDPS loads the FCW and the PC together, so the
    // program's mode takes effect with its first instruction.
    uint8_t seq[22 + 32 + 4];
    const uint16_t org16 = _parkedAt;
    const uint16_t block = org16 + 22;
    const uint16_t ps = block + 32;
    // clang-format off
    const uint8_t code[22] = {
        0x21, 0x00, hi(_nsp), lo(_nsp),             // LD R0, #nsp
        0x7D, 0x0F,                                 // LDCTL NSP, R0
        0x21, 0x00, hi(_psap), lo(_psap),           // LD R0, #psap
        0x7D, 0x0D,                                 // LDCTL PSAP, R0
        0x5C, 0x01, 0x00, 0x0F, hi(block), lo(block),  // LDM R0, block, #16
        0x79, 0x00, hi(ps), lo(ps),                 // LDPS ps
    };
    // clang-format on
    auto len = 0u;
    for (auto b : code)
        seq[len++] = b;
    for (auto i = 0; i < 16; ++i) {
        seq[len++] = hi(_r[i]);
        seq[len++] = lo(_r[i]);
    }
    seq[len++] = hi(_fcw);
    seq[len++] = lo(_fcw);
    seq[len++] = hi(_pc);
    seq[len++] = lo(_pc);
    auto org = _parkedAt;
    _pins->execInst(seq, len, org, _pc);
    park(_pc, org);
}

void RegsZ8000::helpRegisters() const {
    cli.println("?Reg: PC FCW NSP PSAP R0-R15 RH0-RH7 RL0-RL7 RR0-RR14");
}

constexpr const char *REGS16[] = {
        "R0",
        "R1",
        "R2",
        "R3",
        "R4",
        "R5",
        "R6",
        "R7",  // 1-8
        "R8",
        "R9",
        "R10",
        "R11",
        "R12",
        "R13",
        "R14",
        "R15",  // 9-16
        "PC",
        "FCW",
        "NSP",
        "PSAP",  // 17-20
};
constexpr const char *REGS8[] = {
        "RH0",
        "RH1",
        "RH2",
        "RH3",
        "RH4",
        "RH5",
        "RH6",
        "RH7",  // 21-28
        "RL0",
        "RL1",
        "RL2",
        "RL3",
        "RL4",
        "RL5",
        "RL6",
        "RL7",  // 29-36
};
constexpr const char *REGS32[] = {
        "RR0",
        "RR2",
        "RR4",
        "RR6",
        "RR8",
        "RR10",
        "RR12",
        "RR14",  // 37-44
};

const Regs::RegList *RegsZ8000::listRegisters(uint_fast8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS16, 20, 1, UINT16_MAX},
            {REGS8, 16, 21, UINT8_MAX},
            {REGS32, 8, 37, UINT32_MAX},
    };
    return n < 3 ? &REG_LIST[n] : nullptr;
}

bool RegsZ8000::setRegister(uint_fast8_t reg, uint32_t value) {
    // R15, alone or as RR14's low word, is the program's view.
    if (reg == 16) {
        setR15(value);
    } else if (reg == 44) {
        _r[14] = value >> 16;
        setR15(value);
    } else if (reg >= 1 && reg <= 16) {
        _r[reg - 1] = value;
    } else if (reg == 17) {
        _pc = value;
        return true;
    } else if (reg == 18) {
        _fcw = value;
    } else if (reg == 19) {
        _nsp = value;
    } else if (reg == 20) {
        _psap = value & 0xFF00;
    } else if (reg >= 21 && reg <= 28) {
        auto &r = _r[reg - 21];
        r = uint16(value, lo(r));
    } else if (reg >= 29 && reg <= 36) {
        auto &r = _r[reg - 29];
        r = uint16(hi(r), value);
    } else if (reg >= 37 && reg <= 44) {
        const auto i = (reg - 37) * 2;
        _r[i] = value >> 16;
        _r[i + 1] = value;
    }
    return false;
}

}  // namespace z8000
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
