#include "regs_i8086.h"

#include "debugger.h"
#include "inst_i8086.h"
#include "pins_i8086.h"

namespace debugger {
namespace i8086 {
namespace {
// clang-format off
//                              1         2         3         4         5
//                    0123456789012345678901234567890123456789012345678901234567890
const char line1[] = "CS:IP=xxxx:xxxx SS:SP=xxxx:xxxx DS=xxxx ES=xxxx F=ODITSZAPC";
const char line2[] = "AX=xxxx BX=xxxx CX=xxxx DX=xxxx SI=xxxx DI=xxxx BP=xxxx";
// clang-format on

// _r and _s indices, as the encodings number them.
enum Reg : uint8_t { AX = 0, CX, DX, BX, SP, BP, SI, DI };
enum Sreg : uint8_t { ES = 0, CS, SS, DS };

// Where the save sequence stores, as offsets in the park's CS; the
// stores are captured, never in memory.
constexpr uint16_t STORE_R = 0x00;
constexpr uint16_t STORE_S = 0x10;
constexpr uint16_t STORE_F = 0x18;
}  // namespace

RegsI8086::RegsI8086(PinsI8086 *pins)
    : _pins(pins), _buffer1(line1), _buffer2(line2) {}

const char *RegsI8086::cpu() const {
    return _pins->nec() ? "V30" : "8086";
}

const char *RegsI8086::cpuName() const {
    if (_pins->nec())
        return _pins->bus8() ? "V20" : "V30";
    return _pins->bus8() ? "8088" : "8086";
}

uint32_t RegsI8086::nextIp() const {
    return InstI8086::physical(_s[CS], _ip);
}

void RegsI8086::setIp(uint32_t addr) {
    _s[CS] = (addr >> 4) & 0xF000;
    _ip = addr;
}

void RegsI8086::parkAt(uint16_t cs, uint16_t ip) {
    _parkCs = _s[CS] = cs;
    _parkIp = _ip = ip;
    _frame = false;
}

void RegsI8086::parkInTrap(
        uint16_t cs, uint16_t ip, uint16_t flags, uint8_t vector) {
    _parkCs = InstI8086::PARK_CS;
    _parkIp = InstI8086::PARK_IP;
    _s[CS] = cs;
    // INT 3 pushes the address after itself; continuing runs the
    // instruction it stood for.
    _ip = (vector == InstI8086::VEC_BREAK) ? ip - 1 : ip;
    // The single step's TF is the debugger's.
    _flags = (vector == InstI8086::VEC_STEP) ? (flags & ~InstI8086::FLAGS_TF)
                                             : flags;
    _frame = true;
}

void RegsI8086::print() const {
    auto status = _buffer1;
    status.hex16(6, _s[CS]);
    status.hex16(11, _ip);
    status.hex16(22, _s[SS]);
    status.hex16(27, _r[SP]);
    status.hex16(35, _s[DS]);
    status.hex16(43, _s[ES]);
    status.bits(50, _flags, 0x800, "ODITSZ");
    status.bits(56, _flags, 0x010, "A");
    status.bits(57, _flags, 0x004, "P");
    status.bits(58, _flags, 0x001, "C");
    cli.println(status);
    _pins->idle();
    auto regs = _buffer2;
    regs.hex16(3, _r[AX]);
    regs.hex16(11, _r[BX]);
    regs.hex16(19, _r[CX]);
    regs.hex16(27, _r[DX]);
    regs.hex16(35, _r[SI]);
    regs.hex16(43, _r[DI]);
    regs.hex16(51, _r[BP]);
    cli.println(regs);
    _pins->idle();
}

void RegsI8086::save() {
    // MOV CS:[disp16], reg for each register, then FLAGS through the
    // stack, which the overlay keeps; a far jump back to the park ends
    // it and flushes the queue.
    uint8_t seq[8 * 5 + 4 * 5 + 1 + 5 + 5];
    auto len = 0u;
    auto store = [&](uint8_t op, uint8_t reg, uint16_t disp) {
        seq[len++] = 0x2E;  // CS:
        seq[len++] = op;
        seq[len++] = 0x06 | (reg << 3);  // [disp16]
        seq[len++] = lo(disp);
        seq[len++] = hi(disp);
    };
    for (uint8_t i = 0; i < 8; ++i)
        store(0x89, i, STORE_R + 2 * i);  // MOV [disp], r16
    for (uint8_t i = 0; i < 4; ++i)
        store(0x8C, i, STORE_S + 2 * i);  // MOV [disp], sreg
    seq[len++] = InstI8086::PUSHF;
    store(0x8F, 0, STORE_F);  // POP [disp]
    seq[len++] = InstI8086::JMPF;
    seq[len++] = lo(_parkIp);
    seq[len++] = hi(_parkIp);
    seq[len++] = lo(_parkCs);
    seq[len++] = hi(_parkCs);

    auto &overlay = _pins->overlay();
    overlay.clear();
    const auto park = InstI8086::physical(_parkCs, _parkIp);
    auto org = park;
    _pins->execInst(seq, len, org, park);
    auto stored = [&](uint16_t disp) {
        return overlay.wordAt(InstI8086::physical(_parkCs, disp));
    };
    for (uint8_t i = 0; i < 8; ++i)
        _r[i] = stored(STORE_R + 2 * i);
    for (uint8_t i = 0; i < 4; ++i) {
        // The program's CS is the frame's, or the park.
        if (i != CS)
            _s[i] = stored(STORE_S + 2 * i);
    }
    if (_frame) {
        // The program's SP is above the frame the debugger's trap pushed.
        _r[SP] += 6;
        _frame = false;
    } else {
        _flags = stored(STORE_F);
    }
}

bool RegsI8086::probeVSeries() {
    // clang-format off
    const uint8_t seq[] = {
        0xB8, 0x01, 0x01,                               // MOV AX, 0101H
        0xD5, 0x10,                                     // AAD 16
        0x2E, 0x89, 0x06, lo(STORE_R), hi(STORE_R),     // MOV CS:[0], AX
        InstI8086::JMPF, lo(_parkIp), hi(_parkIp),      // JMP park
                lo(_parkCs), hi(_parkCs),
    };
    // clang-format on
    auto &overlay = _pins->overlay();
    overlay.clear();
    const auto park = InstI8086::physical(_parkCs, _parkIp);
    auto org = park;
    _pins->execInst(seq, sizeof(seq), org, park);
    // 1*16+1 on Intel's, 1*10+1 on NEC's.
    return lo(overlay.wordAt(InstI8086::physical(_parkCs, STORE_R))) == 0x0B;
}

void RegsI8086::restore(bool step) {
    // The segment and general registers by MOV, SS and SP together (MOV
    // SS holds interrupts off for one instruction), then IRET from a
    // frame the overlay holds below the program's SP.
    const uint16_t sp = _r[SP] - 6;
    const uint16_t flags = step ? (_flags | InstI8086::FLAGS_TF) : _flags;
    auto &overlay = _pins->overlay();
    overlay.clear();
    const uint16_t frame[] = {_ip, _s[CS], flags};
    for (uint8_t i = 0; i < 3; ++i) {
        overlay.put(InstI8086::physical(_s[SS], sp + 2 * i), lo(frame[i]));
        overlay.put(InstI8086::physical(_s[SS], sp + 2 * i + 1), hi(frame[i]));
    }
    uint8_t seq[40];
    auto len = 0u;
    auto movImm = [&](uint8_t reg, uint16_t v) {
        seq[len++] = 0xB8 | reg;  // MOV r16, imm16
        seq[len++] = lo(v);
        seq[len++] = hi(v);
    };
    auto movSreg = [&](uint8_t sreg) {
        seq[len++] = 0x8E;  // MOV sreg, AX
        seq[len++] = 0xC0 | (sreg << 3);
    };
    movImm(AX, _s[ES]);
    movSreg(ES);
    movImm(AX, _s[DS]);
    movSreg(DS);
    for (uint8_t i = CX; i <= DI; ++i) {
        if (i != SP)
            movImm(i, _r[i]);
    }
    movImm(AX, _s[SS]);
    movSreg(SS);
    movImm(SP, sp);
    movImm(AX, _r[AX]);
    seq[len++] = InstI8086::IRET;
    auto org = InstI8086::physical(_parkCs, _parkIp);
    _pins->execInst(seq, len, org, PinsI8086::EXIT_OVERLAY);
}

void RegsI8086::helpRegisters() const {
    cli.println("?Reg: AX BX CX DX SP BP SI DI CS DS SS ES IP F AL-DH");
}

constexpr const char *REGS16[] = {
        "AX",
        "CX",
        "DX",
        "BX",
        "SP",
        "BP",
        "SI",
        "DI",  // 1-8
        "ES",
        "CS",
        "SS",
        "DS",  // 9-12
        "IP",
        "F",  // 13-14
};
constexpr const char *REGS8[] = {
        "AL",
        "CL",
        "DL",
        "BL",
        "AH",
        "CH",
        "DH",
        "BH",  // 15-22
};

const Regs::RegList *RegsI8086::listRegisters(uint_fast8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS16, 14, 1, UINT16_MAX},
            {REGS8, 8, 15, UINT8_MAX},
    };
    return n < 2 ? &REG_LIST[n] : nullptr;
}

bool RegsI8086::setRegister(uint_fast8_t reg, uint32_t value) {
    if (reg >= 1 && reg <= 8) {
        _r[reg - 1] = value;
    } else if (reg >= 9 && reg <= 12) {
        _s[reg - 9] = value;
        return reg == 10;
    } else if (reg == 13) {
        _ip = value;
        return true;
    } else if (reg == 14) {
        _flags = value;
    } else if (reg >= 15 && reg <= 18) {
        auto &r = _r[reg - 15];
        r = uint16(hi(r), value);
    } else if (reg >= 19 && reg <= 22) {
        auto &r = _r[reg - 19];
        r = uint16(value, lo(r));
    }
    return false;
}

}  // namespace i8086
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
