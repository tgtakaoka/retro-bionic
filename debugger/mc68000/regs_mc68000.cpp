#include "regs_mc68000.h"

#include "debugger.h"
#include "inst_mc68000.h"
#include "mems_mc68000.h"
#include "pins_mc68000_base.h"

namespace debugger {
namespace mc68000 {
namespace {
// clang-format off
//                              1         2         3         4         5
//                    012345678901234567890123456789012345678901234567890123456
const char line1[] = "PC=xxxxxx SR=xxxx T S I=x XNZVC SSP=xxxxxxxx USP=xxxxxxxx";
const char line2[] = "D=xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx";
const char line3[] = "A=xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx";
// clang-format on

// Captured bytes by address: the longs a push stored, whatever order the
// bus moved their bytes in.
struct Captured {
    const MemsMc68000 *mems;
    const uint8_t *buf;
    const uint32_t *addrs;
    uint_fast8_t n;

    bool readByte(uint32_t addr, uint8_t &v) const {
        for (uint_fast8_t i = 0; i < n; ++i) {
            if (addrs[i] == mems->wrap(addr)) {
                v = buf[i];
                return true;
            }
        }
        return false;
    }
    bool readWord(uint32_t addr, uint16_t &v) const {
        uint8_t h, l;
        if (!readByte(addr, h) || !readByte(addr + 1, l))
            return false;
        v = static_cast<uint16_t>(h) << 8 | l;
        return true;
    }
    bool readLong(uint32_t addr, uint32_t &v) const {
        uint16_t h, l;
        if (!readWord(addr, h) || !readWord(addr + 2, l))
            return false;
        v = static_cast<uint32_t>(h) << 16 | l;
        return true;
    }
};
// The restore's instructions, built word by word.
struct Sequence {
    uint8_t buf[6 + 2 + 6 * 8 + 6 * 8 + 2];
    uint_fast8_t len = 0;

    void word(uint16_t v) {
        buf[len++] = v >> 8;
        buf[len++] = v;
    }
    void immediate(uint16_t opcode, uint32_t v) {
        word(opcode);
        word(v >> 16);
        word(v);
    }
};
}  // namespace

RegsMc68000::RegsMc68000(PinsMc68000Base *pins, MemsMc68000 *mems)
    : _pins(pins),
      _mems(mems),
      _pc(InstMc68000::ORG_PARK),
      _parkedAddr(InstMc68000::ORG_PARK),
      _vecAddr(0),
      _sr(InstMc68000::SR_RESET),
      _ssp(0),
      _usp(0),
      _d{0, 0, 0, 0, 0, 0, 0, 0},
      _a{0, 0, 0, 0, 0, 0, 0},
      _buffer1(line1),
      _buffer2(line2),
      _buffer3(line3) {}

void RegsMc68000::print() const {
    auto b1 = _buffer1;
    b1.hex24(3, _pc);
    b1.hex16(13, _sr);
    b1.bits(18, _sr, InstMc68000::SR_T, "T");
    b1.bits(20, _sr, InstMc68000::SR_S, "S");
    b1.hex4(24, (_sr >> 8) & 7);
    b1.bits(26, _sr, 0x10, "XNZVC");
    b1.hex32(36, _ssp);
    b1.hex32(49, _usp);
    cli.println(b1);
    _pins->idle();
    auto b2 = _buffer2;
    for (uint_fast8_t i = 0; i < 8; ++i)
        b2.hex32(2 + 9 * i, _d[i]);
    cli.println(b2);
    _pins->idle();
    auto b3 = _buffer3;
    for (uint_fast8_t i = 0; i < 7; ++i)
        b3.hex32(2 + 9 * i, _a[i]);
    cli.println(b3);
    _pins->idle();
}

// MOVEM stacks D0-D7/A0-A6 from SSP down, D0 lowest; USP and SR follow.
// BRA back to the origin fetches it again once all have run. SR is the
// program's only when the CPU was parked at its own fetch, after reset;
// otherwise the exception frame holds it.
void RegsMc68000::save() {
    // clang-format off
    static constexpr uint8_t SAVE[] = {
        0x48, 0xE7, 0xFF, 0xFE,  // MOVEM.L D0-D7/A0-A6,-(A7)
        0x4E, 0x68,              // MOVE.L  USP,A0
        0x2F, 0x08,              // MOVE.L  A0,-(A7)
        0x40, 0xE7,              // MOVE    SR,-(A7)
        0x60, 0xF4,              // BRA.S   org
    };
    // clang-format on
    static_assert(SAVE[sizeof(SAVE) - 1] == uint8_t(-sizeof(SAVE)),
            "BRA.S org: the displacement is from the BRA plus 2");
    constexpr uint_fast8_t MOVEM_BYTES = 15 * 4;
    constexpr uint_fast8_t BYTES = MOVEM_BYTES + 4 + 2;
    if (_vector && !_pins->answerVector(_vecAddr, InstMc68000::ORG_PARK)) {
        cli.println("?save: vector not answered");
        return;
    }
    auto org = _vector ? InstMc68000::ORG_PARK : _parkedAddr;
    uint8_t buf[BYTES];
    uint32_t addrs[BYTES];
    const auto n = _pins->captureWrites(SAVE, sizeof(SAVE), buf, addrs,
            sizeof(buf), org, PinsMc68000Base::EXIT_ORG);
    if (n < sizeof(buf) || _pins->cutShort()) {
        cli.println("?save: registers not pushed");
        return;
    }
    const Captured c{_mems, buf, addrs, n};
    // SR went last, as one word: its address is the lower of the last two.
    const auto low = addrs[n - 2] < addrs[n - 1] ? addrs[n - 2] : addrs[n - 1];
    const auto sp = _mems->wrap(low + BYTES);  // SSP as the save began
    for (uint_fast8_t i = 0; i < 8; ++i)
        c.readLong(sp - MOVEM_BYTES + 4 * i, _d[i]);
    for (uint_fast8_t i = 0; i < 7; ++i)
        c.readLong(sp - MOVEM_BYTES + 32 + 4 * i, _a[i]);
    c.readLong(sp - MOVEM_BYTES - 4, _usp);
    if (!_vector) {
        // Parked at the program's own fetch: the push was its SSP and SR.
        _pc = _parkedAddr;
        _ssp = sp;
        c.readWord(low, _sr);
    }
    // Either way the CPU is parked at |org| again, outside a vector.
    _vector = false;
    _parkedAddr = org;
}

void RegsMc68000::reset() {
    _ssp = _mems->read32(InstMc68000::VEC_RESET);
    _pc = _mems->read32(InstMc68000::VEC_RESET + 4) & ~1;
    _sr = InstMc68000::SR_RESET;
}

// Every register comes back as an immediate, USP through A0 first; then
// A7 points at a frame staged just below SSP, which RTE pulls as real
// memory. Exceptions write there anyway, so it is free stack. The
// MC68010's RTE reads a format/vector word too: format 0, a short frame.
void RegsMc68000::restore(bool trace) {
    if (_vector && !_pins->answerVector(_vecAddr, InstMc68000::ORG_PARK)) {
        cli.println("?restore: vector not answered");
        return;
    }
    const auto frame = _mems->wrap(_ssp - frameBytes());
    const auto sr = trace ? (_sr | InstMc68000::SR_T) : _sr;
    const auto stageWord = [this](uint32_t addr, uint16_t v) {
        _mems->write_byte(_mems->wrap(addr), v >> 8);
        _mems->write_byte(_mems->wrap(addr + 1), v & 0xFF);
    };
    stageWord(frame, sr);
    stageWord(frame + 2, _pc >> 16);
    stageWord(frame + 4, _pc);
    if (_mc68010)
        stageWord(frame + 6, 0x0000);
    // MOVEA.L #usp,A0; MOVE A0,USP; MOVE.L #Dn x8; MOVEA.L #An x7, A7 last;
    // RTE.
    Sequence seq;
    constexpr uint16_t MOVE_L_IMM_D = 0x203C;   // MOVE.L #,Dn: n in bits 11-9
    constexpr uint16_t MOVEA_L_IMM_A = 0x207C;  // MOVEA.L #,An
    constexpr uint16_t MOVE_A0_USP = 0x4E60;
    seq.immediate(MOVEA_L_IMM_A, _usp);
    seq.word(MOVE_A0_USP);
    for (uint_fast8_t i = 0; i < 8; ++i)
        seq.immediate(MOVE_L_IMM_D | i << 9, _d[i]);
    for (uint_fast8_t i = 0; i < 7; ++i)
        seq.immediate(MOVEA_L_IMM_A | i << 9, _a[i]);
    seq.immediate(MOVEA_L_IMM_A | 7 << 9, frame);
    seq.word(InstMc68000::RTE);
    auto org = _vector ? InstMc68000::ORG_PARK : _parkedAddr;
    _pins->execInst(seq.buf, seq.len, org, _pc);
    _vector = false;
    _parkedAddr = org;  // the fetch of the PC, as the bus showed it
}

void RegsMc68000::helpRegisters() const {
    cli.println("?Reg: PC SR CCR D0-D7 A0-A6 SSP USP");
}

namespace {
constexpr const char *REGS8[] = {
        "CCR",  // 1
};
constexpr const char *REGS16[] = {
        "SR",  // 2
};
constexpr const char *REGS24[] = {
        "PC",  // 3
};
constexpr const char *REGS32[] = {
        "D0",   // 4
        "D1",   // 5
        "D2",   // 6
        "D3",   // 7
        "D4",   // 8
        "D5",   // 9
        "D6",   // 10
        "D7",   // 11
        "A0",   // 12
        "A1",   // 13
        "A2",   // 14
        "A3",   // 15
        "A4",   // 16
        "A5",   // 17
        "A6",   // 18
        "SSP",  // 19
        "USP",  // 20
};
}  // namespace

const Regs::RegList *RegsMc68000::listRegisters(uint_fast8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS8, 1, 1, UINT8_MAX},
            {REGS16, 1, 2, UINT16_MAX},
            {REGS24, 1, 3, 0xFFFFFF},
            {REGS32, 17, 4, UINT32_MAX},
    };
    return n < sizeof(REG_LIST) / sizeof(REG_LIST[0]) ? &REG_LIST[n] : nullptr;
}

bool RegsMc68000::setRegister(uint_fast8_t reg, uint32_t value) {
    if (reg == 1) {
        _sr = (_sr & 0xFF00) | (value & 0x1F);
    } else if (reg == 2) {
        _sr = value;
    } else if (reg == 3) {
        // Instructions are words: PC stays even.
        _pc = _mems->wrap(value) & ~1;
        return true;
    } else if (reg >= 4 && reg < 12) {
        _d[reg - 4] = value;
    } else if (reg >= 12 && reg < 19) {
        _a[reg - 12] = value;
    } else if (reg == 19) {
        _ssp = value;
    } else if (reg == 20) {
        _usp = value;
    }
    return false;
}

}  // namespace mc68000
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
